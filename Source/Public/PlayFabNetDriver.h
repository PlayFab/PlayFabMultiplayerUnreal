//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "IpNetDriver.h"
#include "OnlineSubsystemPlayFabPackage.h"
#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_PLAYSTATION)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <PartyPal.h>
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH || OSS_PLAYFAB_PLAYSTATION
#include <Party.h>
#include "PlayFabNetDriver.generated.h"

using namespace Party;

class FNetworkNotify;

UCLASS(transient, config=Engine)
class UPlayFabNetDriver : public UIpNetDriver
{
	GENERATED_UCLASS_BODY()

public:

	// UIpNetDriver Interface
	virtual class ISocketSubsystem* GetSocketSubsystem() override;
	virtual bool IsAvailable() const override;
	virtual bool InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error) override;
	virtual bool InitListen(FNetworkNotify* InNotify, FURL& LocalURL, bool bReuseAddressAndPort, FString& Error) override;
	virtual bool InitConnect(FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error) override;
	virtual void Shutdown() override;
	virtual bool IsNetResourceValid() override { return true; }
	virtual void TickDispatch(float DeltaTime) override;

	class FOnlineSubsystemPlayFab* GetOnlineSubsystemPlayFab();
	class FPlayFabSocketSubsystem* GetPlayFabSocketSubsystem();

protected:

	friend class FPlayFabSocketSubsystem;
};