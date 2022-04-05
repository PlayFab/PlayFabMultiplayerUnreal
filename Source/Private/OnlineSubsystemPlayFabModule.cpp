//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "OnlineSubsystemPlayFabModule.h"
#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemPlayFab.h"
#include "HAL/PlatformProcess.h"

IMPLEMENT_MODULE(FOnlineSubsystemPlayFabModule, OnlineSubsystemPlayFab);

 // Class responsible for creating instance(s) of the subsystem
class FOnlineFactoryPlayFab : public IOnlineFactory
{

private:

	// Single instantiation of the PlayFab interface
	FOnlineSubsystemPlayFabPtr& GetSingleton() const
	{
		static FOnlineSubsystemPlayFabPtr PlayFabSingleton;
		return PlayFabSingleton;
	}

	virtual void DestroySubsystem()
	{
		FOnlineSubsystemPlayFabPtr& PlayFabSingleton = GetSingleton();
		if (PlayFabSingleton.IsValid())
		{
			PlayFabSingleton->Shutdown();
			PlayFabSingleton.Reset();
		}
	}

public:

	FOnlineFactoryPlayFab() {}
	virtual ~FOnlineFactoryPlayFab() 
	{
		DestroySubsystem();
	}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName) override
	{
		FOnlineSubsystemPlayFabPtr& PlayFabSingleton = GetSingleton();
		if (PlayFabSingleton.IsValid())
		{
			UE_LOG_ONLINE(Warning, TEXT("Can't create more than one instance of PlayFab online subsystem!"));
			return nullptr;
		}

		PlayFabSingleton = MakeShared<FOnlineSubsystemPlayFab, ESPMode::ThreadSafe>(InstanceName);
		if (PlayFabSingleton->IsEnabled())
		{
			if (!PlayFabSingleton->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("PlayFab API failed to initialize!"));
				DestroySubsystem();
				return nullptr;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("PlayFab API disabled!"));
			DestroySubsystem();
			return nullptr;
		}

		return PlayFabSingleton;
	}
};

void FOnlineSubsystemPlayFabModule::StartupModule()
{
	// Create and register our singleton factory with the main online subsystem for easy access
	PlayFabFactory = new FOnlineFactoryPlayFab();

	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(PLAYFAB_SUBSYSTEM, PlayFabFactory);
}

void FOnlineSubsystemPlayFabModule::ShutdownModule()
{
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(PLAYFAB_SUBSYSTEM);

	delete PlayFabFactory;
	PlayFabFactory = nullptr;
}