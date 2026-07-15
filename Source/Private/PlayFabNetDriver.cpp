//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "PlayFabNetDriver.h"
#include "SocketSubsystem.h"
#include "PlayFabSocketSubsystem.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineSubsystemPlayFabPrivate.h"
#include "OnlineSubsystemSessionSettings.h"
#include "PlayFabSocket.h"

FOnFillCustomMulticastPayload UPlayFabNetDriver::FillCustomMulticastPayloadDelegate;
FOnCustomDataReceived UPlayFabNetDriver::OnCustomDataReceivedDelegate;

UPlayFabNetDriver::UPlayFabNetDriver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

class ISocketSubsystem* UPlayFabNetDriver::GetSocketSubsystem()
{
	return bFallbackToPlatformSocketSubsystem ? ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM) : ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM);
}

bool UPlayFabNetDriver::IsAvailable() const
{
	IOnlineSubsystem* OSSPlayFab = IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM);
	if (OSSPlayFab && !bFallbackToPlatformSocketSubsystem)
	{
		ISocketSubsystem* PlayFabSocketSubsystem = ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM);
		if (PlayFabSocketSubsystem)
		{
			return true;
		}
	}

	return false;
}

bool UPlayFabNetDriver::InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error)
{
	if (bFallbackToPlatformSocketSubsystem)
	{
		UE_LOG(LogSockets, Verbose, TEXT("PlayFabNetDriver::InitBase: URL = %s bFallbackToPlatformSocketSubsystem = true; Returning early Super::InitBase instead of setting up PlayFab Sockets."), *URL.Host);
		return Super::InitBase(bInitAsClient, InNotify, URL, bReuseAddressAndPort, Error);
	}

	if (FOnlineSubsystemPlayFab* OSSPlayFab = GetOnlineSubsystemPlayFab())
	{
		if (FPlayFabSocketSubsystem* SocketSubsystem = GetPlayFabSocketSubsystem())
		{
			FUniqueSocket NewSocket = CreateSocketForProtocol(FNetworkProtocolTypes::PlayFab);

			SetSocketAndLocalAddress(TSharedPtr<FSocket>(NewSocket.Release(), FSocketDeleter(GetSocketSubsystem())));

			SocketSubsystem->LinkNetDriver(this);

			if (UNetDriver::InitBase(bInitAsClient, InNotify, URL, bReuseAddressAndPort, Error))
			{
				OnCustomDataMessageReceivedDelegateHandle = OSSPlayFab->AddOnCustomDataMessageReceivedDelegate_Handle(
					FOnCustomDataMessageReceivedDelegate::CreateUObject(this, &UPlayFabNetDriver::OnCustomDataMessageReceived));
				return true;
			}
		}
	}

	return false;
}

bool UPlayFabNetDriver::InitListen(FNetworkNotify* InNotify, FURL& LocalURL, bool bReuseAddressAndPort, FString& Error)
{
	return Super::InitListen(InNotify, LocalURL, bReuseAddressAndPort, Error);
}

bool UPlayFabNetDriver::InitConnect(FNetworkNotify* InNotify, const FURL& InConnectURL, FString& Error)
{
	if (InConnectURL.Host.StartsWith(TEXT("0.0.0.0")))
	{
		UE_LOG(LogSockets, Verbose, TEXT("PlayFabNetDriver::InitConnect: URL = %s bFallbackToPlatformSocketSubsystem = false"), *InConnectURL.Host);
		bFallbackToPlatformSocketSubsystem = false;
	}
	else
	{
		UE_LOG(LogSockets, Verbose, TEXT("PlayFabNetDriver::InitConnect: URL = %s bFallbackToPlatformSocketSubsystem = true; Connecting to a dedicated server."), *InConnectURL.Host);
		bFallbackToPlatformSocketSubsystem = true;
	}

	return Super::InitConnect(InNotify, InConnectURL, Error);
}

void UPlayFabNetDriver::Shutdown()
{
	UE_LOG(LogSockets, Verbose, TEXT("PlayFabNetDriver: Shutdown called on netdriver"));
	if (OnCustomDataMessageReceivedDelegateHandle.IsValid())
	{
		if (FOnlineSubsystemPlayFab* OSSPlayFab = GetOnlineSubsystemPlayFab())
		{
			OSSPlayFab->ClearOnCustomDataMessageReceivedDelegate_Handle(OnCustomDataMessageReceivedDelegateHandle);
		}
		OnCustomDataMessageReceivedDelegateHandle.Reset();
	}

	Super::Shutdown();
}

void UPlayFabNetDriver::TickDispatch(float DeltaTime)
{
	Super::TickDispatch(DeltaTime);

	TPair<FString, TArray<uint8>> CustomDataPacket;
	while (PendingCustomDataPackets.Dequeue(CustomDataPacket))
	{
		OnCustomDataReceivedDelegate.ExecuteIfBound(GetWorld(), this, CustomDataPacket.Key, CustomDataPacket.Value);
	}
}

void UPlayFabNetDriver::TickFlush(float DeltaSeconds)
{
	Super::TickFlush(DeltaSeconds);

	if (IsServer() && !bFallbackToPlatformSocketSubsystem)
	{
		SendCustomMulticastDataForTick();
	}
}

void UPlayFabNetDriver::SendCustomMulticastDataForTick()
{
	FOnlineSubsystemPlayFab* OSSPlayFab = GetOnlineSubsystemPlayFab();
	if (OSSPlayFab == nullptr || OSSPlayFab->CustomDataLocalEndpoint == nullptr || !FillCustomMulticastPayloadDelegate.IsBound())
	{
		return;
	}

	TArray<uint8> Payload;
	if (FillCustomMulticastPayloadDelegate.Execute(GetWorld(), this, Payload) && Payload.Num() > 0)
	{
		OSSPlayFab->MulticastCustomData(Payload);
	}
}

void UPlayFabNetDriver::OnCustomDataMessageReceived(const Party::PartyEndpointMessageReceivedStateChange* Change)
{
	if (Change == nullptr || Change->senderEndpoint == nullptr || Change->messageBuffer == nullptr || Change->messageSize == 0)
	{
		return;
	}

	PartyString SenderEntityId = nullptr;
	PartyError Err = Change->senderEndpoint->GetEntityId(&SenderEntityId);
	if (PARTY_FAILED(Err) || SenderEntityId == nullptr)
	{
		UE_LOG(LogSockets, Warning, TEXT("UPlayFabNetDriver::OnCustomDataMessageReceived: Failed to resolve sender EntityId: %s"), *GetPartyErrorMessage(Err));
		return;
	}

	const uint8* Buffer = static_cast<const uint8*>(Change->messageBuffer);
	TArray<uint8> Payload(Buffer, Change->messageSize);
	PendingCustomDataPackets.Enqueue(TPair<FString, TArray<uint8>>(UTF8_TO_TCHAR(SenderEntityId), MoveTemp(Payload)));
}

FOnlineSubsystemPlayFab* UPlayFabNetDriver::GetOnlineSubsystemPlayFab()
{
	return static_cast<FOnlineSubsystemPlayFab*>(IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM));
}

FPlayFabSocketSubsystem* UPlayFabNetDriver::GetPlayFabSocketSubsystem()
{
	return static_cast<FPlayFabSocketSubsystem*>(GetSocketSubsystem());
}