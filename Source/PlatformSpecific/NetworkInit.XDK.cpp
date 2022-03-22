//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HAL/Platform.h"
PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS

#if OSS_PLAYFAB_XBOXONE
#include "PlatformDefines.h"

#include "OnlineSubsystemPlayFab.h"

#include <PartyImpl.h>
#include <PartyTypes.h>
#include "PartyXboxLive.h"
#include "PartyXboxLiveImpl.h"

using namespace Party;

void FOnlineSubsystemPlayFab::RegisterNetworkInitCallbacks()
{
	// no-op no XDK
}

void FOnlineSubsystemPlayFab::UnregisterNetworkInitCallbacks()
{
	// no-op on XDK
}

void FOnlineSubsystemPlayFab::TryInitializePlayFabParty()
{
	// Initialize PlayFab Party immediately on XDK
	InitializePlayFabParty();
}

#endif