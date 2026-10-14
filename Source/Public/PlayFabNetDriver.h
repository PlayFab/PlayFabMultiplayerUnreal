//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "IpNetDriver.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "Containers/Queue.h"
#include "PlayFabNetDriver.generated.h"

class FNetworkNotify;
namespace Party
{
	struct PartyEndpointMessageReceivedStateChange;
}

/** Supplies the byte-identical custom payload sent by a listen server for the current tick. */
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnFillCustomMulticastPayload, UWorld* /*World*/, UNetDriver* /*NetDriver*/, TArray<uint8>& /*OutPayload*/);

/** Receives a custom multicast payload on a client. */
DECLARE_DELEGATE_FourParams(FOnCustomDataReceived, UWorld* /*World*/, UNetDriver* /*NetDriver*/, const FString& /*SenderEntityId*/, const TArray<uint8>& /*Payload*/);

UCLASS(transient, config=Engine)
class ONLINESUBSYSTEMPLAYFAB_API UPlayFabNetDriver : public UIpNetDriver
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
	virtual void TickFlush(float DeltaSeconds) override;

	/** Bind once at module startup to provide the listen server's payload for each tick. */
	static FOnFillCustomMulticastPayload FillCustomMulticastPayloadDelegate;

	/** Bind once at module startup to consume custom multicast payloads on clients. */
	static FOnCustomDataReceived OnCustomDataReceivedDelegate;

	class FOnlineSubsystemPlayFab* GetOnlineSubsystemPlayFab();
	class FPlayFabSocketSubsystem* GetPlayFabSocketSubsystem();

private:
	bool bFallbackToPlatformSocketSubsystem = false;

	void SendCustomMulticastDataForTick();
	void OnCustomDataMessageReceived(const Party::PartyEndpointMessageReceivedStateChange* Change);
	FDelegateHandle OnCustomDataMessageReceivedDelegateHandle;
	TQueue<TPair<FString, TArray<uint8>>> PendingCustomDataPackets;

protected:
	friend class UPlayFabNetConnection;
	friend class FPlayFabSocketSubsystem;
};