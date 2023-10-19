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
	if (bFallbackToPlatformSocketSubsystem)
	{
		// TODO: Find a way to get the correct one directly or do one return per platform with the hardcoded strings
		return ISocketSubsystem::Get(FName(TEXT("WINDOWS")));
	}
	else
	{
		return ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM);
	}
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