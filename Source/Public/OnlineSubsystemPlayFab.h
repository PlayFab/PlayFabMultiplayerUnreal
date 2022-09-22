//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "OnlineDelegateMacros.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "OnlineVoiceInterfacePlayFab.h"
#include "OnlineCognitiveServicesInterfacePlayFab.h"
#include "OnlineSubsystemPlayFabDefines.h"
#ifdef OSS_PLAYFAB_PLAYSTATION
#include "OnlineAsyncTaskPlayFab.h"
#endif // OSS_PLAYFAB_PLAYSTATION
#include "Misc/ConfigCacheIni.h"
#include "..\PlatformSpecific\PlatformDefines.h"
#include "..\Private\PlayFabEventTracer.h"

THIRD_PARTY_INCLUDES_START
#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_PLAYSTATION)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <PartyPal.h>
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH || OSS_PLAYFAB_PLAYSTATION
#include <Party.h>
#include <PFEntityKey.h>
#include <PFMultiplayer.h>
#include <PFLobby.h>
THIRD_PARTY_INCLUDES_END

using namespace Party;

#define OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(INTERFACE_TYPE, INTERFACE_NAME)\
INTERFACE_TYPE FOnlineSubsystemPlayFab::INTERFACE_NAME() const\
{ \
	IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();\
	return NativeSubsystem ? NativeSubsystem->INTERFACE_NAME() : nullptr; \
}

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineSessionPlayFab, ESPMode::ThreadSafe> FOnlineSessionPlayFabPtr;
typedef TSharedPtr<class FOnlineIdentityPlayFab, ESPMode::ThreadSafe> FOnlineIdentityPlayFabPtr;
typedef TSharedPtr<class FOnlineExternalUIPlayFab, ESPMode::ThreadSafe> FOnlineExternalUIPlayFabPtr;
typedef TSharedPtr<class FOnlineVoicePlayFab, ESPMode::ThreadSafe> FOnlineVoicePlayFabPtr;
typedef TSharedPtr<class FOnlineCognitiveServicesPlayFab, ESPMode::ThreadSafe> FOnlineCognitiveServicesPlayFabPtr;
typedef TSharedPtr<class FPlayFabLobby, ESPMode::ThreadSafe> FPlayFabLobbyPtr;
typedef TSharedPtr<class FMatchmakingInterfacePlayFab, ESPMode::ThreadSafe> FMatchmakingInterfacePtr;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEndpointMessageReceived, const PartyEndpointMessageReceivedStateChange* /*Change*/);
typedef FOnEndpointMessageReceived::FDelegate FOnEndpointMessageReceivedDelegate;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnectToPlayFabPartyNetworkCompleted, bool /*bSuccess*/);
typedef FOnConnectToPlayFabPartyNetworkCompleted::FDelegate FOnConnectToPlayFabPartyNetworkCompletedDelegate;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPartyEndpointCreated, bool /*bSuccess*/, uint16 /*EndpointID*/, bool /*bIsHosting*/);
typedef FOnPartyEndpointCreated::FDelegate FOnPartyEndpointCreatedDelegate;

enum class EPlayFabPartyNetworkState
{
	NoNetwork,
	JoiningNetwork_Host,
	JoiningNetwork_Host_PendingEndpointCreation,
	JoiningNetwork_Client,
	NetworkReady,
	LeavingNetwork
};

 //OnlineSubsystemPlayFab - Implementation of the online subsystem for PLAYFAB services
class ONLINESUBSYSTEMPLAYFAB_API FOnlineSubsystemPlayFab : public FOnlineSubsystemImpl
{
PACKAGE_SCOPE:

	// Only the factory makes instances
	FOnlineSubsystemPlayFab() = delete;
	FOnlineSubsystemPlayFab(FName InInstanceName) :
		FOnlineSubsystemImpl(PLAYFAB_SUBSYSTEM, InInstanceName)
	{
		// Fix subsystem name to the base one
		FString NativePlatformServiceName;
		if (!GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("NativePlatformService"), NativePlatformServiceName, GEngineIni))
		{
			UE_LOG_ONLINE(Error, TEXT("Engine INI OnlineSubsystem section does not contain a value for key NativePlatformService"));
		}
		else
		{
			SubsystemName = FName(*NativePlatformServiceName);
		}
	}

#ifdef OSS_PLAYFAB_PLAYSTATION
	FOnlineAsyncTaskManagerPlayFab* GetAsyncTaskManager() { return OnlineAsyncTaskThreadRunnable; }
	FRunnableThread* OnlineAsyncTaskThread;
#endif // OSS_PLAYFAB_PLAYSTATION

public:

	virtual ~FOnlineSubsystemPlayFab()
	{
	}

	// IOnlineSubsystem

	// Implemented (partially or fully)
	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual FOnlineCognitiveServicesPlayFabPtr GetCognitiveServicesInterface() const;

	// Passthrough
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
	virtual IOnlineStatsPtr GetStatsInterface() const override;
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
	virtual IOnlineTournamentPtr GetTournamentInterface() const override;

	/** Helpers to get typed Interface shared pointers */
	FOnlineSessionPlayFabPtr GetSessionInterfacePlayFab() const { return SessionInterface; }

	virtual bool IsLocalPlayer(const FUniqueNetId& UniqueId) const override;
	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual bool IsEnabled() const override;
	virtual FString GetAppId() const override;
	virtual FText GetOnlineServiceName() const override;

	void OnAppSuspend();
	void OnAppResume();

	// FTickerObjectBase
	virtual bool Tick(float DeltaTime) override;

	// Chat <-> Network bridge
	bool AddChatControlToNetwork(PartyLocalChatControl* LocalChatControl);
	
	bool CreateAndConnectToPlayFabPartyNetwork();
	bool ConnectToPlayFabPartyNetwork(const FString& NetworkId, const FString& NetworkDescriptorStr);
	void LeavePlayFabPartyNetwork();

	bool bNetworkInitialized = false;
	bool bPartyInitialized = false;
	bool bMultiplayerInitialized = false;
	bool bMemoryCallbacksSet = false;

	int32 MaxDeviceCount = 8;
	int32 MaxDevicesPerUserCount = 1;
	int32 MaxEndpointsPerDeviceCount = 1;
	int32 MaxUserCount = 8;
	int32 MaxUsersPerDeviceCount = 1;
	PartyDirectPeerConnectivityOptions DirectPeerConnectivityOptions 
		= PartyDirectPeerConnectivityOptions::AnyPlatformType | PartyDirectPeerConnectivityOptions::AnyEntityLoginProvider;

	EPlayFabPartyNetworkState NetworkState = EPlayFabPartyNetworkState::NoNetwork;
	PartyNetwork* Network = nullptr;
	PartyLocalEndpoint* LocalEndpoint = nullptr;
	TMap<uint32, PartyEndpoint*> Endpoints;
	
	FString NetworkId;
	PartyNetworkDescriptor NetworkDescriptor;

	FString SerializeNetworkDescriptor(const PartyNetworkDescriptor& InNetworkDescriptor);

	FPlayFabLobbyPtr GetPlayFabLobbyInterface() const;
	FMatchmakingInterfacePtr GetMatchmakingInterface() const;
	
private:
	void RegisterNetworkInitCallbacks();
	void UnregisterNetworkInitCallbacks();
	void TryInitializePlayFabParty();
	void InitializePlayFabParty();
	void InitializeMultiplayer();
	void InitializeEventTracer();
	void SetMemoryCallbacks();
	void CleanUpPlayFab();

#ifdef OSS_PLAYFAB_SWITCH
	void* LibBufferParty;
	void* LibBufferMultiplayer;
	void* PartyDllHandle;
	void* MultiplayerDllHandle;

	void CleanUpSwitch();
#endif // OSS_PLAYFAB_SWITCH

	void ParseDirectPeerConnectivityOptions();
	
	void DoWork();

	bool InternalConnectToNetwork(PartyLocalUser* PlayFabPartyLocalUser, const FString& InNetworkId, Party::PartyNetworkDescriptor& NetworkDescriptor);

	FOnlineIdentityPlayFabPtr IdentityInterface;
	FOnlineSessionPlayFabPtr SessionInterface;
	FOnlineExternalUIPlayFabPtr ExternalUIInterface;
	FOnlineVoicePlayFabPtr VoiceInterface;
	FOnlineCognitiveServicesPlayFabPtr CognitiveServicesInterface;
	FPlayFabLobbyPtr PlayFabLobbyInterface;
	FMatchmakingInterfacePtr MatchmakingInterface;
	TUniquePtr<PlayFabEventTracer> EventTracer;

	PFMultiplayerHandle MultiplayerHandle;

#ifdef OSS_PLAYFAB_PLAYSTATION
	/** Online async task runnable */
	FOnlineAsyncTaskManagerPlayFab* OnlineAsyncTaskThreadRunnable;
#endif // OSS_PLAYFAB_PLAYSTATION

#ifdef OSS_PLAYFAB_PLAYSTATION
	void* PartyDllHandle;
	void* MultiplayerDllHandle;
	bool InitPlayStationNpTitleId();
	void DeinitPlayStationNpTitleId();
#if !UE_BUILD_SHIPPING
	void SetPlayStationNpTitleId();
#endif // !UE_BUILD_SHIPPING
#endif // OSS_PLAYFAB_PLAYSTATION
	
public:

	PFMultiplayerHandle GetMultiplayerHandle() {return MultiplayerHandle;}
	void OnRegionsChanged(const PartyStateChange* Change);
	void OnDestroyLocalUserCompleted(const PartyStateChange* Change);
	void OnCreateNewNetworkCompleted(const PartyStateChange* Change);
	void OnConnectToNetworkCompleted(const PartyStateChange* Change);
	void OnAuthenticateLocalUserCompleted(const PartyStateChange* Change);
	void OnNetworkConfigurationMadeAvailable(const PartyStateChange* Change);
	void OnNetworkDescriptorChanged(const PartyStateChange* Change);
	void OnLocalUserRemoved(const PartyStateChange* Change);
	void OnRemoveLocalUserCompleted(const PartyStateChange* Change);
	void OnLocalUserKicked(const PartyStateChange* Change);
	void OnCreateEndpointCompleted(const PartyStateChange* Change);
	void OnDestroyEndpointCompleted(const PartyStateChange* Change);
	void OnEndpointCreated(const PartyStateChange* Change);
	void OnEndpointDestroyed(const PartyStateChange* Change);
	void OnRemoteDeviceCreated(const PartyStateChange* Change);
	void OnRemoteDeviceDestroyed(const PartyStateChange* Change);
	void OnRemoteDeviceJoinedNetwork(const PartyStateChange* Change);
	void OnRemoteDeviceLeftNetwork(const PartyStateChange* Change);
	void OnDevicePropertiesChanged(const PartyStateChange* Change);
	void OnLeaveNetworkCompleted(const PartyStateChange* Change);
	void OnNetworkDestroyed(const PartyStateChange* Change);
	void OnEndpointMessageReceived(const PartyStateChange* Change);
	void OnDataBuffersReturned(const PartyStateChange* Change);
	void OnEndpointPropertiesChanged(const PartyStateChange* Change);
	void OnSynchronizeMessagesBetweenEndpointsCompleted(const PartyStateChange* Change);
	void OnCreateInvitationCompleted(const PartyStateChange* Change);
	void OnRevokeInvitationCompleted(const PartyStateChange* Change);
	void OnInvitationCreated(const PartyStateChange* Change);
	void OnInvitationDestroyed(const PartyStateChange* Change);
	void OnNetworkPropertiesChanged(const PartyStateChange* Change);
	void OnKickDeviceCompleted(const PartyStateChange* Change);
	void OnKickUserCompleted(const PartyStateChange* Change);
	void OnSetChatAudioEncoderBitrateCompleted(const PartyStateChange* Change);
	void OnLocalChatAudioOutputChanged(const PartyStateChange* Change);
	void OnSetLanguageCompleted(const PartyStateChange* Change);
	void OnChatControlPropertiesChanged(const PartyStateChange* Change);
	void OnDisconnectChatControlCompleted(const PartyStateChange* Change);
	void OnConfigureAudioManipulationVoiceStreamCompleted(const PartyStateChange* Change);
	void OnConfigureAudioManipulationCaptureStreamCompleted(const PartyStateChange* Change);
	void OnConfigureAudioManipulationRenderStreamCompleted(const PartyStateChange* Change);

	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnEndpointMessageReceived, const PartyEndpointMessageReceivedStateChange* /*Change*/);
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnConnectToPlayFabPartyNetworkCompleted, bool /*bSuccess*/);
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnPartyEndpointCreated, bool /*bSuccess*/, uint16 /*EndpointID*/, bool /*bIsHosting*/);
	
	PartyEndpoint* GetPartyEndpoint(uint32 EndpointId);

	#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	const FString GetSandBox() const;
	#endif // OSS_PLAYFAB_WINGDK || OSS_PLAYFAB_XSX || OSS_PLAYFAB_XBOXONEGDK
};

namespace FNetworkProtocolTypes
{
	ONLINESUBSYSTEMPLAYFAB_API extern const FLazyName PlayFab;
}

typedef TSharedPtr<FOnlineSubsystemPlayFab, ESPMode::ThreadSafe> FOnlineSubsystemPlayFabPtr;

FString GetPartyErrorMessage(PartyError InError);
FString GetMultiplayerErrorMessage(HRESULT hr);
FString PartyStateChangeResultToReasonString(PartyStateChangeResult result);
FString GetPartyStateChangeTypeString(PartyStateChangeType Type);
FString GetNetworkStateStateString(EPlayFabPartyNetworkState State);