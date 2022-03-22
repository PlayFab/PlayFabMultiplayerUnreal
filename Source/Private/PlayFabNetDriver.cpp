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

UPlayFabNetDriver::UPlayFabNetDriver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

class ISocketSubsystem* UPlayFabNetDriver::GetSocketSubsystem()
{
	return ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM);
}

bool UPlayFabNetDriver::IsAvailable() const
{
	IOnlineSubsystem* OSSPlayFab = IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM);
	if (OSSPlayFab)
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
	if (FOnlineSubsystemPlayFab* OSSPlayFab = GetOnlineSubsystemPlayFab())
	{
		if (FPlayFabSocketSubsystem* SocketSubsystem = GetPlayFabSocketSubsystem())
		{
			FUniqueSocket NewSocket = CreateSocketForProtocol(FNetworkProtocolTypes::PlayFab);
			SetSocketAndLocalAddress(NewSocket.Release());

			SocketSubsystem->LinkNetDriver(this);

			if (UNetDriver::InitBase(bInitAsClient, InNotify, URL, bReuseAddressAndPort, Error))
			{
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
	return Super::InitConnect(InNotify, InConnectURL, Error);
}

void UPlayFabNetDriver::Shutdown()
{
	UE_LOG(LogSockets, Verbose, TEXT("PlayFabNetDriver: Shutdown called on netdriver"));

	Super::Shutdown();
}

void UPlayFabNetDriver::TickDispatch(float DeltaTime)
{
	Super::TickDispatch(DeltaTime);
}

FOnlineSubsystemPlayFab* UPlayFabNetDriver::GetOnlineSubsystemPlayFab()
{
	return static_cast<FOnlineSubsystemPlayFab*>(IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM));
}

FPlayFabSocketSubsystem* UPlayFabNetDriver::GetPlayFabSocketSubsystem()
{
	return static_cast<FPlayFabSocketSubsystem*>(GetSocketSubsystem());
}