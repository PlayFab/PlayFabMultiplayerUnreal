//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "HAL/Platform.h"
PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS

#if OSS_PLAYFAB_WIN64
#include "OnlineSessionSettings.h"
#include "PlatformDefines.h"
#include "OnlineSubsystemPlayFab.h"

void FOnlineSubsystemPlayFab::RegisterNetworkInitCallbacks()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::RegisterNetworkInitCallbacks"));
}

void FOnlineSubsystemPlayFab::UnregisterNetworkInitCallbacks()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::UnregisterNetworkInitCallbacks"));
}

void FOnlineSubsystemPlayFab::TryInitializePlayFabParty()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::TryInitializePlayFabParty"));
	InitializePlayFabParty();
}

#endif