//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "PlayFabNetConnection.h"
#include "PlayFabSocketSubsystem.h"
#include "PlayFabNetDriver.h"
#include "Net/DataChannel.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemPlayFabDefines.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemPlayfabPrivate.h"

#include "Runtime/Launch/Resources/Version.h"

UPlayFabNetConnection::UPlayFabNetConnection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPlayFabNetConnection::InitLocalConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	UPlayFabNetDriver* PlayFabNetDriver = Cast<UPlayFabNetDriver>(InDriver);
	if (PlayFabNetDriver)
	{
		bFallbackToPlatformSocketSubsystem = PlayFabNetDriver->bFallbackToPlatformSocketSubsystem;
		if (bFallbackToPlatformSocketSubsystem)
		{
			UE_LOG_ONLINE(Verbose, TEXT("UPlayFabNetConnection::InitLocalConnection bFallbackToPlatformSocketSubsystem = true; Connecting to a dedicated server and early returning."));
			Super::InitLocalConnection(InDriver, InSocket, InURL, InState, InMaxPacket, InPacketOverhead);
			return;
		}
	}

	DisableAddressResolution();
	PlayerId.SetUniqueNetId(nullptr);

	FString HostConnectInfo;
	IOnlineSubsystem* OSSPlayFab = IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM);
	if (OSSPlayFab)
	{
		FNamedOnlineSession* Session = OSSPlayFab->GetSessionInterface()->GetNamedSession(NAME_GameSession);
		if (Session)
		{
			Session->SessionSettings.Get(SETTING_HOST_CONNECT_INFO, HostConnectInfo);
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("UPlayFabNetConnection::InitLocalConnection failed: Named Session GameSession is null"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("UPlayFabNetConnection::InitLocalConnection failed: Online Subsystem is null"));
	}

	InitBase(InDriver, InSocket, InURL, InState, InMaxPacket, InPacketOverhead);

	FPlayFabSocketSubsystem* SocketSubsystem = static_cast<FPlayFabSocketSubsystem*>(InDriver->GetSocketSubsystem());
	check(SocketSubsystem);

	RemoteAddr = InDriver->GetSocketSubsystem()->CreateInternetAddr();
	bool isValid = false;
	RemoteAddr->SetIp(*HostConnectInfo, isValid);

	FString PortStr;
	if (HostConnectInfo.StartsWith(PLAYFAB_URL_PREFIX))
	{
		PortStr = HostConnectInfo.Mid(UE_ARRAY_COUNT(PLAYFAB_URL_PREFIX) - 1);
	}

	const int32 Port = FCString::Atoi(*PortStr);
	RemoteAddr->SetPort(Port);

	InitSendBuffer();
}

void UPlayFabNetConnection::InitRemoteConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, const class FInternetAddr& InRemoteAddr, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	DisableAddressResolution();
	PlayerId.SetUniqueNetId(nullptr);

	InitBase(InDriver, InSocket, InURL, InState, InMaxPacket, InPacketOverhead);

	RemoteAddr = InRemoteAddr.Clone();

	URL.Host = RemoteAddr->ToString(false);

	InitSendBuffer();

	SetClientLoginState(EClientLoginState::LoggingIn);
	SetExpectedClientLoginMsgType(NMT_Hello);
}

void UPlayFabNetConnection::FlushNet(bool bIgnoreSimulation)
{
	#if ENGINE_MAJOR_VERSION >= 5
	const EConnectionState CurState = UNetConnection::GetConnectionState();
	if (!bFallbackToPlatformSocketSubsystem && (CurState == USOCK_Closed || CurState == USOCK_Invalid))
	#else
	if (!bFallbackToPlatformSocketSubsystem && (State == USOCK_Closed || State == USOCK_Invalid))
	#endif
	{
		SendBuffer.Reset();
		InitSendBuffer();
		return;
	}
	else
	{
		Super::FlushNet(bIgnoreSimulation);
	}
}