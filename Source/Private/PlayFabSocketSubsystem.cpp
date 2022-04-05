//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "PlayFabSocketSubsystem.h"
#include "OnlineSubsystemPlayFab.h"
#include "PlayFabSocket.h"
#include "SocketSubsystemModule.h"
#include "PlayFabSocketSubsystem.h"
#include "IPAddressPlayFab.h"

FPlayFabSocketSubsystem* FPlayFabSocketSubsystem::SocketSingleton = nullptr;

 // Create the socket subsystem for the given platform service
FName CreatePlayFabSocketSubsystem()
{
	FName SubsystemName(PLAYFAB_SOCKET_SUBSYSTEM);

	// Create and register our singleton factory with the main online subsystem for easy access
	FPlayFabSocketSubsystem* SocketSubsystem = FPlayFabSocketSubsystem::Create();
	FString Error;
	if (SocketSubsystem->Init(Error))
	{
		FSocketSubsystemModule& SSS = FModuleManager::LoadModuleChecked<FSocketSubsystemModule>("Sockets");
		SSS.RegisterSocketSubsystem(SubsystemName, SocketSubsystem, true);
		return SubsystemName;
	}
	else
	{
		FPlayFabSocketSubsystem::Destroy();
		return NAME_None;
	}
}

 // Tear down the socket subsystem for the given platform service
void DestroyPlayFabSocketSubsystem()
{
	FModuleManager& ModuleManager = FModuleManager::Get();

	if (ModuleManager.IsModuleLoaded("Sockets"))
	{
		FSocketSubsystemModule& SSS = FModuleManager::GetModuleChecked<FSocketSubsystemModule>("Sockets");
		SSS.UnregisterSocketSubsystem(PLAYFAB_SOCKET_SUBSYSTEM);
	}

	FPlayFabSocketSubsystem::Destroy();
}

FPlayFabSocketSubsystem::FPlayFabSocketSubsystem()
{
}

FPlayFabSocketSubsystem* FPlayFabSocketSubsystem::Create()
{
	if (SocketSingleton == nullptr)
	{
		SocketSingleton = new FPlayFabSocketSubsystem();
	}

	return SocketSingleton;
}

void FPlayFabSocketSubsystem::Destroy()
{
	if (SocketSingleton)
	{
		SocketSingleton->Shutdown();
		delete SocketSingleton;
		SocketSingleton = nullptr;
	}
}

void FPlayFabSocketSubsystem::OnEndpointMessageReceived(const PartyEndpointMessageReceivedStateChange* MessagedReceivedChange)
{
	if (MessagedReceivedChange)
	{
		const uint8* Buffer = static_cast<const uint8*>(MessagedReceivedChange->messageBuffer);
		TArray<uint8> Payload(Buffer, MessagedReceivedChange->messageSize);

		uint16 SenderEndpointId = 0;
		PartyError Err = MessagedReceivedChange->senderEndpoint->GetUniqueIdentifier(&SenderEndpointId);
		if (PARTY_SUCCEEDED(Err))
		{
			if (FPlayFabSocket* Socket = GetSocket())
			{
				Socket->AddNewPendingData(SenderEndpointId, Payload);
			}
		}
		else
		{
			UE_LOG(LogSockets, Warning, TEXT("FPlayFabSocketSubsystem::OnEndpointMessageReceived: GetEntityId failed:  %s"), *GetPartyErrorMessage(Err));
		}
	}
}

bool FPlayFabSocketSubsystem::Init(FString& Error)
{
	return true;
}

void FPlayFabSocketSubsystem::RegisterDelegates(FOnlineSubsystemPlayFab* InOSSPlayFab)
{
	if (InOSSPlayFab)
	{
		OSSPlayFab = InOSSPlayFab;

		OnEndpointMessageReceivedDelegateHandle = OSSPlayFab->AddOnEndpointMessageReceivedDelegate_Handle(FOnEndpointMessageReceivedDelegate::CreateRaw(this, &FPlayFabSocketSubsystem::OnEndpointMessageReceived));
	}
}

void FPlayFabSocketSubsystem::Shutdown()
{
	UE_LOG(LogSockets, Log, TEXT("FPlayFabSocketSubsystem: Shutting down"));

	CleanUpActiveSockets();

	// Clean up our delegates here
	if (OSSPlayFab)
	{
		OSSPlayFab->ClearOnEndpointMessageReceivedDelegate_Handle(OnEndpointMessageReceivedDelegateHandle);
	}

	OSSPlayFab = nullptr;
}

class FSocket* FPlayFabSocketSubsystem::CreateSocket(const FName& SocketType, const FString& SocketDescription, const FName& ProtocolType)
{
	UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocketSubsystem::CreateSocket"));

	FPlayFabSocket* NewSocket = nullptr;

	if (OSSPlayFab)
	{
		NewSocket = new FPlayFabSocket(OSSPlayFab, SocketDescription, FNetworkProtocolTypes::PlayFab);
		AddSocket(NewSocket);
	}

	return NewSocket;
}

void FPlayFabSocketSubsystem::DestroySocket(class FSocket* Socket)
{
	UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocketSubsystem::DestroySocket"));

	FPlayFabSocket* PlayFabSocket = static_cast<FPlayFabSocket*>(Socket);
	if (PlayFabSocket)
	{
		RemoveSocket(PlayFabSocket);
		delete Socket;
	}
}

FAddressInfoResult FPlayFabSocketSubsystem::GetAddressInfo(const TCHAR* HostName, const TCHAR* ServiceName /*= nullptr*/, EAddressInfoFlags QueryFlags /*= EAddressInfoFlags::Default*/, const FName ProtocolTypeName /*= NAME_None*/, ESocketType SocketType /*= ESocketType::SOCKTYPE_Unknown*/)
{
	UE_LOG_ONLINE(Warning, TEXT("FPlayFabSocketSubsystem::GetAddressInfo is not supported on PlayFab Sockets"));
	return FAddressInfoResult(HostName, ServiceName);
}

TSharedPtr<FInternetAddr> FPlayFabSocketSubsystem::GetAddressFromString(const FString& IPAddress)
{
	UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocketSubsystem::GetAddressFromString"));

	FInternetAddrPlayFab* Addr = new FInternetAddrPlayFab();
	bool IsValid;
	Addr->SetIp(*IPAddress, IsValid);
	return MakeShareable(Addr);
}

TSharedRef<FInternetAddr> FPlayFabSocketSubsystem::CreateInternetAddr()
{
	return MakeShareable(new FInternetAddrPlayFab());
}

bool FPlayFabSocketSubsystem::GetLocalAdapterAddresses(TArray<TSharedPtr<FInternetAddr>>& OutAddresses)
{
	bool bCanBindAll;

	OutAddresses.Add(GetLocalHostAddr(*GLog, bCanBindAll));

	return true;
}

TSharedRef<FInternetAddr> FPlayFabSocketSubsystem::GetLocalBindAddr(FOutputDevice& Out)
{
	if (OSSPlayFab && OSSPlayFab->LocalEndpoint)
	{
		uint16 EndpointId = 0;
		PartyError Err = OSSPlayFab->LocalEndpoint->GetUniqueIdentifier(&EndpointId);

		if (PARTY_FAILED(Err))
		{
			UE_LOG_ONLINE(Warning, TEXT("FPlayFabSocketSubsystem::GetLocalBindAddr: failed: %s"), *GetPartyErrorMessage(Err));
		}
		else
		{
			return MakeShareable(new FInternetAddrPlayFab(EndpointId));
		}
	}

	return MakeShareable(new FInternetAddrPlayFab());
}

TSharedRef<FInternetAddr> FPlayFabSocketSubsystem::GetLocalHostAddr(FOutputDevice& Out, bool& bCanBindAll)
{
	TSharedRef<FInternetAddr> HostAddr = GetLocalBindAddr(Out);
	bCanBindAll = true;

	return HostAddr;
}

const TCHAR* FPlayFabSocketSubsystem::GetSocketAPIName() const
{
	return TEXT("PlayFab");
}

FPlayFabSocket* FPlayFabSocketSubsystem::GetSocket()
{
	UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocketSubsystem::GetSocket"));

	FPlayFabSocket* FoundSocket = nullptr;
	for (FPlayFabSocket* Socket : ActiveSockets)
	{
		if (Socket)
		{
			FoundSocket = Socket;
			break;
		}
	}

	if (FoundSocket == nullptr)
	{
		UE_LOG(LogSockets, Warning, TEXT("FPlayFabSocketSubsystem::GetSocketByEntityId: Cannot get Socket, returning null"));
	}

	return FoundSocket;
}

void FPlayFabSocketSubsystem::AddSocket(FPlayFabSocket* NewSocket)
{
	UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocketSubsystem::AddSocket"));

	if (NewSocket)
	{
		ActiveSockets.Add(NewSocket);
	}
	else
	{
		UE_LOG(LogSockets, Warning, TEXT("FPlayFabSocketSubsystem::AddSocket: NewSocket was null, cannot add socket"));
	}
}

void FPlayFabSocketSubsystem::RemoveSocket(FPlayFabSocket* Socket)
{
	UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocketSubsystem::RemoveSocket"));

	ActiveSockets.RemoveSingleSwap(Socket);
}

void FPlayFabSocketSubsystem::CleanUpActiveSockets()
{
	UE_LOG(LogSockets, Verbose, TEXT("FPlayFabSocketSubsystem::CleanUpActiveSockets"));

	// Clean up sockets
	TArray<FPlayFabSocket*> TempArray = ActiveSockets;
	for (int SocketIdx = 0; SocketIdx < TempArray.Num(); SocketIdx++)
	{
		DestroySocket(TempArray[SocketIdx]);
	}

	ActiveSockets.Empty();
}

void FPlayFabSocketSubsystem::LinkNetDriver(UPlayFabNetDriver* InNetDriver)
{
	NetDriver = InNetDriver;
}