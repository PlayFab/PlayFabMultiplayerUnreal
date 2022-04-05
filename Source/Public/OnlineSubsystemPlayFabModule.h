//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Online subsystem module class  (PLAYFAB Implementation)
 * Code related to the loading of the PLAYFAB module
 */
class FOnlineSubsystemPlayFabModule : public IModuleInterface
{
private:

	// Class responsible for creating instance(s) of the subsystem
	class FOnlineFactoryPlayFab* PlayFabFactory;

public:

	FOnlineSubsystemPlayFabModule()
        : PlayFabFactory(nullptr)
	{}

	virtual ~FOnlineSubsystemPlayFabModule() {}

	// IModuleInterface

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	virtual bool SupportsAutomaticShutdown() override
	{
		return false;
	}
};
