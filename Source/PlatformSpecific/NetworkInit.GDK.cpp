//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HAL/Platform.h"
PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
#include "PlatformDefines.h"

#include "OnlineSubsystemPlayFab.h"

#include "XNetworking.h"
#include <XGameRuntimeFeature.h>

namespace
{
	XTaskQueueRegistrationToken NetworkInitRegistration;
}

void FOnlineSubsystemPlayFab::RegisterNetworkInitCallbacks()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::RegisterNetworkInitCallbacks"));

	XNetworkingRegisterConnectivityHintChanged(nullptr, this, [](void* Context, const XNetworkingConnectivityHint* ConnectivityHint)
	{
		FOnlineSubsystemPlayFab* OSS = reinterpret_cast<FOnlineSubsystemPlayFab*>(Context);
		if (OSS)
		{
			// if the new value is different than the old value
			if (ConnectivityHint->networkInitialized != OSS->bNetworkInitialized)
			{
				UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab: ConnectivityHint.networkInitialized changed: %s"), ConnectivityHint->networkInitialized ? TEXT("true") : TEXT("false"));

				OSS->bNetworkInitialized = ConnectivityHint->networkInitialized;

				if (OSS->bNetworkInitialized)
				{
					OSS->InitializePlayFabParty();
				}
			}
		}
	}, 
	& NetworkInitRegistration);
}

void FOnlineSubsystemPlayFab::UnregisterNetworkInitCallbacks()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::UnregisterNetworkInitCallbacks"));

	XNetworkingUnregisterConnectivityHintChanged(NetworkInitRegistration, true);
}

void FOnlineSubsystemPlayFab::TryInitializePlayFabParty()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::TryInitializePlayFabParty"));

	if (XGameRuntimeIsFeatureAvailable(XGameRuntimeFeature::XNetworking))
	{
		XNetworkingConnectivityHint ConnectivityHint;

		auto Hr = XNetworkingGetConnectivityHint(&ConnectivityHint);

		if (SUCCEEDED(Hr))
		{
			if (ConnectivityHint.networkInitialized)
			{
				bNetworkInitialized = true;

				InitializePlayFabParty();
			}
			else
			{
				UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::TryInitializePlayFabParty: ConnectivityHint.networkInitialized was false, waiting to initialize PlayFab Party"));
			}
		}
	}
}
#endif