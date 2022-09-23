//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "OnlineSubsystemPlayFab.h"
#include "OnlineIdentityInterfacePlayFab.h"
#include "PlayFabSocketSubsystem.h"
#include "Interfaces/IPluginManager.h"
#include "SocketSubsystem.h"
#include "IpConnection.h"
#include "PlayFabNetDriver.h"

#include "EngineLogs.h"
#include "CoreGlobals.h"
#include "OnlineSessionInterfacePlayFab.h"
#include "PlayFabLobby.h"
#include "MatchmakingInterfacePlayFab.h"
#include "Engine/Engine.h"

THIRD_PARTY_INCLUDES_START
#ifdef OSS_PLAYFAB_SWITCH
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH
#include <PartyImpl.h>
#include <PartyTypes.h>
#include <PFLobby.h>
THIRD_PARTY_INCLUDES_END

OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineFriendsPtr, GetFriendsInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlinePartyPtr, GetPartyInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineGroupsPtr, GetGroupsInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineSharedCloudPtr, GetSharedCloudInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineUserCloudPtr, GetUserCloudInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineLeaderboardsPtr, GetLeaderboardsInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineTimePtr, GetTimeInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineTitleFilePtr, GetTitleFileInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineEntitlementsPtr, GetEntitlementsInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineStoreV2Ptr, GetStoreV2Interface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlinePurchasePtr, GetPurchaseInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineEventsPtr, GetEventsInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineAchievementsPtr, GetAchievementsInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineSharingPtr, GetSharingInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineUserPtr, GetUserInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineMessagePtr, GetMessageInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlinePresencePtr, GetPresenceInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineChatPtr, GetChatInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineStatsPtr, GetStatsInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineTurnBasedPtr, GetTurnBasedInterface);
OSS_PLAYFAB_PASSTHROUGH_FUNCTION_DEFINITION(IOnlineTournamentPtr, GetTournamentInterface);

namespace
{
	void* PlayFabAlloc(size_t Size, uint32_t MemoryTypeId)
	{
		return FMemory::Malloc(Size);
	}

	void PlayFabFree(void* PointerToMemory, uint32_t MemoryTypeId)
	{
		FMemory::Free(PointerToMemory);
	}
}

namespace FNetworkProtocolTypes
{
	const FLazyName PlayFab(TEXT("PlayFab"));
}

bool FOnlineSubsystemPlayFab::Init()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::Init"));
	
#ifdef OSS_PLAYFAB_PLAYSTATION
	OnlineAsyncTaskThreadRunnable = new FOnlineAsyncTaskManagerPlayFab(this);
	check(OnlineAsyncTaskThreadRunnable);
	OnlineAsyncTaskThread = FRunnableThread::Create(OnlineAsyncTaskThreadRunnable, *FString::Printf(TEXT("OnlineAsyncTaskThreadPlayFab %s"), *InstanceName.ToString()));
	check(OnlineAsyncTaskThread);
	UE_LOG_ONLINE(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());
#endif // OSS_PLAYFAB_PLAYSTATION

#ifdef OSS_PLAYFAB_PLAYSTATION
	if (!InitPlayStationNpTitleId())
	{
		return false;
	}
#if !UE_BUILD_SHIPPING
	SetPlayStationNpTitleId();
#endif // !UE_BUILD_SHIPPING
#endif // OSS_PLAYFAB_PLAYSTATION

	// Register for suspend/resume callbacks
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddRaw(this, &FOnlineSubsystemPlayFab::OnAppSuspend);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddRaw(this, &FOnlineSubsystemPlayFab::OnAppResume);
	
	RegisterNetworkInitCallbacks();

	// Try to initialize PlayFab Party if the platform is ready
	TryInitializePlayFabParty();

	GConfig->GetInt(TEXT("OnlineSubsystemPlayFab"), TEXT("MaxDeviceCount"), MaxDeviceCount, GEngineIni);
	GConfig->GetInt(TEXT("OnlineSubsystemPlayFab"), TEXT("MaxDevicesPerUserCount"), MaxDevicesPerUserCount, GEngineIni);
	GConfig->GetInt(TEXT("OnlineSubsystemPlayFab"), TEXT("MaxEndpointsPerDeviceCount"), MaxEndpointsPerDeviceCount, GEngineIni);
	GConfig->GetInt(TEXT("OnlineSubsystemPlayFab"), TEXT("MaxUserCount"), MaxUserCount, GEngineIni);
	GConfig->GetInt(TEXT("OnlineSubsystemPlayFab"), TEXT("MaxUsersPerDeviceCount"), MaxUsersPerDeviceCount, GEngineIni);
	ParseDirectPeerConnectivityOptions();

	TSharedPtr<IPlugin> SocketsPlugin = IPluginManager::Get().FindPlugin(TEXT("PlayFab"));
	if (!SocketsPlugin.IsValid() || (SocketsPlugin.IsValid() && !SocketsPlugin->IsEnabled()))
	{
		UE_LOG_ONLINE(Log, TEXT("Initializing PlayFabPartSocketSubsystem"));
		CreatePlayFabSocketSubsystem();
	}

	if (FPlayFabSocketSubsystem* SocketSubsystem = static_cast<FPlayFabSocketSubsystem*>(ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM)))
	{
		SocketSubsystem->RegisterDelegates(this);
	}

	// Initialize Multiplayer
	InitializeMultiplayer();

	// Construct OSS interfaces such that dependent interfaces are initialized first.
	PlayFabLobbyInterface = MakeShared<FPlayFabLobby, ESPMode::ThreadSafe>(this);
	MatchmakingInterface = MakeShared<FMatchmakingInterfacePlayFab, ESPMode::ThreadSafe>(this);
	IdentityInterface = MakeShared<FOnlineIdentityPlayFab, ESPMode::ThreadSafe>(this);
	SessionInterface = MakeShared<FOnlineSessionPlayFab, ESPMode::ThreadSafe>(this);
	ExternalUIInterface = MakeShared<FOnlineExternalUIPlayFab, ESPMode::ThreadSafe>(this);
	VoiceInterface = MakeShared<FOnlineVoicePlayFab, ESPMode::ThreadSafe>(this);
	CognitiveServicesInterface = MakeShared<FOnlineCognitiveServicesPlayFab, ESPMode::ThreadSafe>(this);

	// Initialize EventTracer
	InitializeEventTracer();

	return true;
}

void FOnlineSubsystemPlayFab::InitializePlayFabParty()
{
	if (!bPartyInitialized)
	{
		UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::InitializePlayFabParty"));

		SetMemoryCallbacks();

		PartyManager& Manager = PartyManager::GetSingleton();

		FString TitleID;
		GConfig->GetString(TEXT("OnlineSubsystemPlayFab"), TEXT("PlayFabTitleID"), TitleID, GEngineIni);

		// Initialize PlayFab Party
		PartyError Err = Manager.Initialize(TCHAR_TO_UTF8(*TitleID));
		if (PARTY_FAILED(Err))
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemPlayFab::InitializePlayFabParty: Initialize failed: %s"), *GetPartyErrorMessage(Err));
			return;
		}

		UE_LOG_ONLINE(Display, TEXT("FOnlineSubsystemPlayFab InitializePlayFabParty with PlayFab TitleID %s"), *TitleID);
		bPartyInitialized = true;
	}
}

void FOnlineSubsystemPlayFab::InitializeMultiplayer()
{
	if (!bMultiplayerInitialized)
	{
		UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::InitializeLobby"));
		
		// SetMemoryCallbacks(); 
		
		// Initialize Lobby
		FString TitleID;
		GConfig->GetString(TEXT("OnlineSubsystemPlayFab"), TEXT("PlayFabTitleID"), TitleID, GEngineIni);
		HRESULT hr = PFMultiplayerInitialize(TCHAR_TO_UTF8(*TitleID), &MultiplayerHandle);
		if (FAILED(hr))
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemPlayFab::PFMultiplayerInitialize failed: 0x%08x"), hr);
			return;
		}

		UE_LOG_ONLINE(Display, TEXT("FOnlineSubsystemPlayFab MultiPlayerManager with PlayFab TitleID %s"), *TitleID);
		bMultiplayerInitialized = true;
	}
}

void FOnlineSubsystemPlayFab::InitializeEventTracer()
{
	EventTracer = MakeUnique<PlayFabEventTracer>(this);
	EventTracer->OnPlayFabMultiplayerManagerInitialize();
}

void FOnlineSubsystemPlayFab::SetMemoryCallbacks()
{
	// Only needs to be called once
	if (!bMemoryCallbacksSet)
	{
		UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::SetMemoryCallbacks"));

		PartyManager& Manager = PartyManager::GetSingleton();

		PartyError Err = Manager.SetMemoryCallbacks(PlayFabAlloc, PlayFabFree);
		if (PARTY_FAILED(Err))
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemPlayFab::SetMemoryCallbacks failed: %s"), *GetPartyErrorMessage(Err));
		}
		
		bMemoryCallbacksSet = true;
	}
}

void FOnlineSubsystemPlayFab::CleanUpPlayFab()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::CleanUpPlayFab PartyInitialized %d, LobbyInitialized %d"), bPartyInitialized, bMultiplayerInitialized);

	if (bPartyInitialized)
	{
		bPartyInitialized = false;

		NetworkState = EPlayFabPartyNetworkState::NoNetwork;
		Network = nullptr;

		// This cleans up everything allocated in PartyManager.Initialize() and
		// should only be used when done with networking
		PartyManager::GetSingleton().Cleanup();
	}

	if (bMultiplayerInitialized)
	{
		bMultiplayerInitialized = false;
		HRESULT Hr = PFMultiplayerUninitialize(MultiplayerHandle);
		if (FAILED(Hr))
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::PFMultiplayerCleanup failed:[0x%08x], %s"),Hr, *GetMultiplayerErrorMessage(Hr));
		}
		MultiplayerHandle = nullptr;
	}
}

const static TMap<FString, PartyDirectPeerConnectivityOptions> ConnectivityOptionsMap  = {
	{"None", PartyDirectPeerConnectivityOptions::None},
	{"SamePlatformType", PartyDirectPeerConnectivityOptions::SamePlatformType},
	{"DifferentPlatformType", PartyDirectPeerConnectivityOptions::DifferentPlatformType},
	{"AnyPlatformType", PartyDirectPeerConnectivityOptions::SamePlatformType |
		PartyDirectPeerConnectivityOptions::DifferentPlatformType},
	{"SameEntityLoginProvider", PartyDirectPeerConnectivityOptions::SameEntityLoginProvider},
	{"DifferentEntityLoginProvider", PartyDirectPeerConnectivityOptions::DifferentEntityLoginProvider},
	{"AnyEntityLoginProvider", PartyDirectPeerConnectivityOptions::SameEntityLoginProvider |
		PartyDirectPeerConnectivityOptions::DifferentEntityLoginProvider},
};

void FOnlineSubsystemPlayFab::ParseDirectPeerConnectivityOptions()
{
	TArray<FString> ConnectivityOptionsArr;
	GConfig->GetArray(TEXT("OnlineSubsystemPlayFab"), TEXT("DirectPeerConnectivityOptions"), ConnectivityOptionsArr, GEngineIni);
	if (!ConnectivityOptionsArr.Num())
	{
		UE_LOG_ONLINE(Warning, TEXT("DirectPeerConnectivityOptions not provided - using default."));
		return; // use default value for DirectPeerConnectivity Options
	}
	
	PartyDirectPeerConnectivityOptions ConnectivityOptions = PartyDirectPeerConnectivityOptions::None;
	for (const FString& ConnType : ConnectivityOptionsArr)
	{
		const PartyDirectPeerConnectivityOptions* ConnectivityOption = ConnectivityOptionsMap.Find(ConnType);
		if (!ConnectivityOption)
		{ // LOG error and exit, default value will be used
			UE_LOG_ONLINE(Error, TEXT("Engine INI OnlineSubsystemPlayFab section contains erroneous value for key DirectPeerConnectivityOptions"));
			return;
		}
		ConnectivityOptions |= *ConnectivityOption;
	}
	// On success, set class variable
	DirectPeerConnectivityOptions = ConnectivityOptions;
}

bool FOnlineSubsystemPlayFab::Shutdown()
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemPlayFab::Shutdown()"));

	FOnlineSubsystemImpl::Shutdown();

	NetworkState = EPlayFabPartyNetworkState::NoNetwork;

#ifdef OSS_PLAYFAB_PLAYSTATION
	if (OnlineAsyncTaskThread)
	{
		delete OnlineAsyncTaskThread;
		OnlineAsyncTaskThread = nullptr;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		delete OnlineAsyncTaskThreadRunnable;
		OnlineAsyncTaskThreadRunnable = nullptr;
	}
#endif // OSS_PLAYFAB_PLAYSTATION

#define DESTRUCT_INTERFACE(Interface) \
	if (Interface.IsValid()) \
	{ \
		ensure(Interface.IsUnique()); \
		Interface = nullptr; \
	}

	// Destruct the interfaces (in opposite order they were created)	
	DESTRUCT_INTERFACE(CognitiveServicesInterface);
	DESTRUCT_INTERFACE(VoiceInterface);
	DESTRUCT_INTERFACE(ExternalUIInterface);
	DESTRUCT_INTERFACE(SessionInterface);
	DESTRUCT_INTERFACE(IdentityInterface);
	DESTRUCT_INTERFACE(MatchmakingInterface);
	DESTRUCT_INTERFACE(PlayFabLobbyInterface);

#undef DESTRUCT_INTERFACE

	DestroyPlayFabSocketSubsystem();

	CleanUpPlayFab();

	UnregisterNetworkInitCallbacks();

	// Unregister for suspend/resume callbacks
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);

#ifdef OSS_PLAYFAB_SWITCH
	CleanUpSwitch();
#endif // OSS_PLAYFAB_SWITCH
#ifdef OSS_PLAYFAB_PLAYSTATION
	DeinitPlayStationNpTitleId();
#endif // OSS_PLAYFAB_PLAYSTATION

	return true;
}

bool FOnlineSubsystemPlayFab::Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FOnlineSubsystemImpl::Exec(InWorld, Cmd, Ar))
	{
		return true;
	}

	bool bWasHandled = false;

	return bWasHandled;
}

bool FOnlineSubsystemPlayFab::IsEnabled() const
{
	// Check the ini for disabling PFP
	bool bEnabled = FOnlineSubsystemImpl::IsEnabled();
	if (bEnabled)
	{
#if UE_EDITOR
		if (bEnabled)
		{
			bEnabled = IsRunningDedicatedServer() || IsRunningGame();
		}
#endif
	}

	return bEnabled;
}

FString FOnlineSubsystemPlayFab::GetAppId() const
{
	FString TitleID;
	GConfig->GetString(TEXT("OnlineSubsystemPlayFab"), TEXT("PlayFabTitleID"), TitleID, GEngineIni);
	return TitleID;
}

FText FOnlineSubsystemPlayFab::GetOnlineServiceName() const
{
	return NSLOCTEXT("OnlineSubsystemPlayFab", "OnlineServiceName", "PlayFab");
}

void FOnlineSubsystemPlayFab::OnAppSuspend()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnAppSuspend"));

	if (IdentityInterface.IsValid())
	{
		IdentityInterface->OnAppSuspend();
	}

	if (VoiceInterface.IsValid())
	{
		VoiceInterface->OnAppSuspend();
	}
	
	CleanUpPlayFab();
}

void FOnlineSubsystemPlayFab::OnAppResume()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnAppResume"));

	TryInitializePlayFabParty();

	if (IdentityInterface.IsValid())
	{
		IdentityInterface->OnAppResume();
	}

	if (VoiceInterface.IsValid())
	{
		VoiceInterface->OnAppResume();
	}
}

bool FOnlineSubsystemPlayFab::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemImpl::Tick(DeltaTime))
	{
		return false;
	}

#ifdef OSS_PLAYFAB_PLAYSTATION
	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->GameTick();
	}
#endif // OSS_PLAYFAB_PLAYSTATION

	if (IdentityInterface.IsValid())
	{
		IdentityInterface->Tick(DeltaTime);
		EventTracer->DoWork();
	}

	DoWork();

	if (VoiceInterface.IsValid())
	{
		VoiceInterface->Tick(DeltaTime);
	}

	if (SessionInterface.IsValid())
	{
		SessionInterface->Tick(DeltaTime);
	}

	if (ExternalUIInterface.IsValid())
	{
		ExternalUIInterface->Tick(DeltaTime);
	}

	return true;
}

IOnlineIdentityPtr FOnlineSubsystemPlayFab::GetIdentityInterface() const
{
	return IdentityInterface;
}

IOnlineSessionPtr FOnlineSubsystemPlayFab::GetSessionInterface() const
{
	return SessionInterface;
}

IOnlineExternalUIPtr FOnlineSubsystemPlayFab::GetExternalUIInterface() const
{
	return ExternalUIInterface;
}

IOnlineVoicePtr FOnlineSubsystemPlayFab::GetVoiceInterface() const
{
	return VoiceInterface;
}

FOnlineCognitiveServicesPlayFabPtr FOnlineSubsystemPlayFab::GetCognitiveServicesInterface() const
{
	return CognitiveServicesInterface;
}

FPlayFabLobbyPtr FOnlineSubsystemPlayFab::GetPlayFabLobbyInterface() const
{
	return PlayFabLobbyInterface;
}

FMatchmakingInterfacePtr FOnlineSubsystemPlayFab::GetMatchmakingInterface() const
{
	return MatchmakingInterface;
}

bool FOnlineSubsystemPlayFab::IsLocalPlayer(const FUniqueNetId& UniqueId) const
{
	if (!IsDedicated())
	{
		IOnlineIdentityPtr IdentityInt = GetIdentityInterface();
		if (IdentityInt.IsValid())
		{
			for (int32 LocalUserNum = 0; LocalUserNum < MAX_LOCAL_PLAYERS; LocalUserNum++)
			{
				TSharedPtr<const FUniqueNetId> LocalUniqueId = IdentityInt->GetUniquePlayerId(LocalUserNum);
				if (LocalUniqueId.IsValid() && UniqueId == *LocalUniqueId)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void FOnlineSubsystemPlayFab::DoWork()
{
	if (!bPartyInitialized)
	{
		return;
	}
	
	uint32_t Count;
	PartyStateChangeArray Changes;
	PartyManager& Manager = PartyManager::GetSingleton();

	// Start processing messages from PlayFab Party
	PartyError Err = Manager.StartProcessingStateChanges(&Count, &Changes);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::DoWork: StartProcessingStateChanges failed: %s"), *GetPartyErrorMessage(Err));
		return;
	}

	for (uint32_t i = 0; i < Count; i++)
	{
		const PartyStateChange* Change = Changes[i];

		if (Change)
		{
			switch (Change->stateChangeType)
			{
			case PartyStateChangeType::RegionsChanged:									 OnRegionsChanged(Change); break;
			case PartyStateChangeType::DestroyLocalUserCompleted:						 OnDestroyLocalUserCompleted(Change); break;
			case PartyStateChangeType::CreateNewNetworkCompleted:						 OnCreateNewNetworkCompleted(Change); break;
			case PartyStateChangeType::ConnectToNetworkCompleted:						 OnConnectToNetworkCompleted(Change); break;
			case PartyStateChangeType::AuthenticateLocalUserCompleted:					 OnAuthenticateLocalUserCompleted(Change); break;
			case PartyStateChangeType::NetworkConfigurationMadeAvailable:				 OnNetworkConfigurationMadeAvailable(Change); break;
			case PartyStateChangeType::NetworkDescriptorChanged:						 OnNetworkDescriptorChanged(Change); break;
			case PartyStateChangeType::LocalUserRemoved:								 OnLocalUserRemoved(Change); break;
			case PartyStateChangeType::RemoveLocalUserCompleted:						 OnRemoveLocalUserCompleted(Change); break;
			case PartyStateChangeType::LocalUserKicked:									 OnLocalUserKicked(Change); break;
			case PartyStateChangeType::CreateEndpointCompleted:							 OnCreateEndpointCompleted(Change); break;
			case PartyStateChangeType::DestroyEndpointCompleted:						 OnDestroyEndpointCompleted(Change); break;
			case PartyStateChangeType::EndpointCreated:									 OnEndpointCreated(Change); break;
			case PartyStateChangeType::EndpointDestroyed:								 OnEndpointDestroyed(Change); break;
			case PartyStateChangeType::RemoteDeviceCreated:								 OnRemoteDeviceCreated(Change); break;
			case PartyStateChangeType::RemoteDeviceDestroyed:							 OnRemoteDeviceDestroyed(Change); break;
			case PartyStateChangeType::RemoteDeviceJoinedNetwork:						 OnRemoteDeviceJoinedNetwork(Change); break;
			case PartyStateChangeType::RemoteDeviceLeftNetwork:							 OnRemoteDeviceLeftNetwork(Change); break;
			case PartyStateChangeType::DevicePropertiesChanged:							 OnDevicePropertiesChanged(Change); break;
			case PartyStateChangeType::LeaveNetworkCompleted:							 OnLeaveNetworkCompleted(Change); break;
			case PartyStateChangeType::NetworkDestroyed:								 OnNetworkDestroyed(Change); break;
			case PartyStateChangeType::EndpointMessageReceived:							 OnEndpointMessageReceived(Change); break;
			case PartyStateChangeType::DataBuffersReturned:								 OnDataBuffersReturned(Change); break;
			case PartyStateChangeType::EndpointPropertiesChanged:						 OnEndpointPropertiesChanged(Change); break;
			case PartyStateChangeType::SynchronizeMessagesBetweenEndpointsCompleted:	 OnSynchronizeMessagesBetweenEndpointsCompleted(Change); break;
			case PartyStateChangeType::CreateInvitationCompleted:						 OnCreateInvitationCompleted(Change); break;
			case PartyStateChangeType::RevokeInvitationCompleted:						 OnRevokeInvitationCompleted(Change); break;
			case PartyStateChangeType::InvitationCreated:								 OnInvitationCreated(Change); break;
			case PartyStateChangeType::InvitationDestroyed:								 OnInvitationDestroyed(Change); break;
			case PartyStateChangeType::NetworkPropertiesChanged:						 OnNetworkPropertiesChanged(Change); break;
			case PartyStateChangeType::KickDeviceCompleted:								 OnKickDeviceCompleted(Change); break;
			case PartyStateChangeType::KickUserCompleted:								 OnKickUserCompleted(Change); break;
			case PartyStateChangeType::ChatControlJoinedNetwork:						 VoiceInterface->OnChatControlJoinedNetwork(Change); break;
			case PartyStateChangeType::ChatControlLeftNetwork:							 VoiceInterface->OnChatControlLeftNetwork(Change); break;
			case PartyStateChangeType::ChatControlDestroyed:							 VoiceInterface->OnChatControlDestroyed(Change); break;
			case PartyStateChangeType::ChatControlCreated:								 VoiceInterface->OnChatControlCreated(Change); break;
			case PartyStateChangeType::ConnectChatControlCompleted:						 VoiceInterface->OnConnectChatControlCompleted(Change); break;
			case PartyStateChangeType::CreateChatControlCompleted:						 VoiceInterface->OnCreateChatControlCompleted(Change); break;
			case PartyStateChangeType::DestroyChatControlCompleted:						 VoiceInterface->OnDestroyChatControlCompleted(Change); break;
			case PartyStateChangeType::LocalChatAudioInputChanged:						 VoiceInterface->OnLocalChatAudioInputChanged(Change); break;
			case PartyStateChangeType::SetChatAudioInputCompleted:						 VoiceInterface->OnSetChatAudioInputCompleted(Change); break;
			case PartyStateChangeType::SetChatAudioOutputCompleted:						 VoiceInterface->OnSetChatAudioOutputCompleted(Change); break;
			case PartyStateChangeType::SetChatAudioEncoderBitrateCompleted:				 OnSetChatAudioEncoderBitrateCompleted(Change); break;
			case PartyStateChangeType::LocalChatAudioOutputChanged:						 OnLocalChatAudioOutputChanged(Change); break;
			case PartyStateChangeType::SetLanguageCompleted:							 OnSetLanguageCompleted(Change); break;
			case PartyStateChangeType::ChatControlPropertiesChanged:					 OnChatControlPropertiesChanged(Change); break;
			case PartyStateChangeType::DisconnectChatControlCompleted:					 OnDisconnectChatControlCompleted(Change); break;
			case PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted:	 OnConfigureAudioManipulationVoiceStreamCompleted(Change); break;
			case PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted: OnConfigureAudioManipulationCaptureStreamCompleted(Change); break;
			case PartyStateChangeType::ConfigureAudioManipulationRenderStreamCompleted:	 OnConfigureAudioManipulationRenderStreamCompleted(Change); break;
			case PartyStateChangeType::ChatTextReceived:								 CognitiveServicesInterface->OnChatTextReceived(Change); break;
			case PartyStateChangeType::VoiceChatTranscriptionReceived:					 CognitiveServicesInterface->OnVoiceChatTranscriptionReceived(Change); break;
			case PartyStateChangeType::SetTextToSpeechProfileCompleted:					 CognitiveServicesInterface->OnSetTextToSpeechProfileCompleted(Change); break;
			case PartyStateChangeType::SynthesizeTextToSpeechCompleted:					 CognitiveServicesInterface->OnSynthesizeTextToSpeechCompleted(Change); break;
			case PartyStateChangeType::SetTranscriptionOptionsCompleted:				 CognitiveServicesInterface->OnSetTranscriptionOptionsCompleted(Change); break;
			case PartyStateChangeType::SetTextChatOptionsCompleted:						 CognitiveServicesInterface->OnSetTextChatOptionsCompleted(Change); break;
			case PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted:	 CognitiveServicesInterface->OnPopulateAvailableTextToSpeechProfilesCompleted(Change); break;
			}
		}
	}
	
	// Return the processed changes back to the PartyManager
	Err = Manager.FinishProcessingStateChanges(Count, Changes);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::DoWork: FinishProcessingStateChanges failed: %s"), *GetPartyErrorMessage(Err));
	}
}

bool FOnlineSubsystemPlayFab::CreateAndConnectToPlayFabPartyNetwork()
{
	UE_LOG_ONLINE(Log, TEXT("FOnlineSubsystemPlayFab::CreateAndConnectToPlayFabPartyNetwork()"));

	if (!bPartyInitialized)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::CreateAndConnectToPlayFabPartyNetwork: Cannot create to playfab network, PartyManager is not initialized."));
		return false;
	}

	if (NetworkState != EPlayFabPartyNetworkState::NoNetwork || Network != nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::CreateAndConnectToPlayFabPartyNetwork: Cannot create to playfab network while already connected to a network."));
		return false;
	}

	PartyLocalUser* FirstPartyLocalUser = IdentityInterface ? IdentityInterface->GetFirstPartyLocalUser() : nullptr;
	if (FirstPartyLocalUser == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::CreateAndConnectToPlayFabPartyNetwork: Could not create new network PlayFabPartyLocalUser was null"));
		return false;
	}

	const FString NewNetworkId = FGuid::NewGuid().ToString();
	const std::string NetworkIdStr = TCHAR_TO_UTF8(*NewNetworkId);

	PartyNetworkConfiguration PlayFabPartyNetworkConfig = {};

	// Setup the network using values from config
	PlayFabPartyNetworkConfig.maxDeviceCount = MaxDeviceCount;				
	PlayFabPartyNetworkConfig.maxDevicesPerUserCount = MaxDevicesPerUserCount;		
	PlayFabPartyNetworkConfig.maxEndpointsPerDeviceCount = MaxEndpointsPerDeviceCount;	
	PlayFabPartyNetworkConfig.maxUserCount = MaxUserCount;					
	PlayFabPartyNetworkConfig.maxUsersPerDeviceCount = MaxUsersPerDeviceCount;
	PlayFabPartyNetworkConfig.directPeerConnectivityOptions = DirectPeerConnectivityOptions;

	// Setup the network invitation configuration to use the network id as an invitation id and allow anyone to join.
	PartyInvitationConfiguration PartyInviteConfig{
		NetworkIdStr.c_str(),					// Invitation identifier
		PartyInvitationRevocability::Anyone,	// Revocability
		0,										// Authorized user count
		nullptr									// Authorized user list
	};

	PartyNetworkDescriptor NewNetworkDescriptor = {};

	// Create a new network descriptor
	PartyError Err = PartyManager::GetSingleton().CreateNewNetwork(
		FirstPartyLocalUser,		// Local User
		&PlayFabPartyNetworkConfig,	// Network Config
		0,							// Region List Count
		nullptr,					// Region List
		&PartyInviteConfig,			// Invitation configuration
		nullptr,					// Async Identifier
		&NewNetworkDescriptor,		// OUT network descriptor
		nullptr						// Applied initial invitation identifier
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::CreateAndConnectToPlayFabPartyNetwork: CreateNewNetwork failed: %s"), *GetPartyErrorMessage(Err));
		return false;
	}

	// Connect to the new network
	if (InternalConnectToNetwork(FirstPartyLocalUser, NewNetworkId, NewNetworkDescriptor))
	{
		NetworkState = EPlayFabPartyNetworkState::JoiningNetwork_Host;
		NetworkId = NewNetworkId;

		return true;
	}

	return false;
}

bool FOnlineSubsystemPlayFab::ConnectToPlayFabPartyNetwork(const FString& NewNetworkId, const FString& NewNetworkDescriptorStr)
{
	UE_LOG_ONLINE(Log, TEXT("FOnlineSubsystemPlayFab::ConnectToPlayFabPartyNetwork()"));

	if (bPartyInitialized == false)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::ConnectToPlayFabPartyNetwork: Cannot connect to playfab network, PartyManager is not initialized."));
		return false;
	}

	if (NetworkState != EPlayFabPartyNetworkState::NoNetwork || Network != nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::ConnectToPlayFabPartyNetwork: Cannot connect to playfab network while already connected to a network."));
		return false;
	}

	PartyLocalUser* FirstPartyLocalUser = IdentityInterface ? IdentityInterface->GetFirstPartyLocalUser() : nullptr;
	if (FirstPartyLocalUser == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::ConnectToPlayFabPartyNetwork: Could not create new network FirstPartyLocalUser was null"));
		return false;
	}

	PartyNetworkDescriptor NewNetworkDescriptor = {};

	// Deserialize the remote network's descriptor
	PartyError Err = PartyManager::DeserializeNetworkDescriptor(TCHAR_TO_UTF8(*NewNetworkDescriptorStr), &NewNetworkDescriptor);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::ConnectToPlayFabPartyNetwork: failed to deserialize descriptor: %s"), *GetPartyErrorMessage(Err));
		return false;
	}

	// Connect to the remote network
	if (InternalConnectToNetwork(FirstPartyLocalUser, NewNetworkId, NewNetworkDescriptor))
	{
		NetworkState = EPlayFabPartyNetworkState::JoiningNetwork_Client;
		NetworkId = NewNetworkId;

		return true;
	}

	return false;
}

bool FOnlineSubsystemPlayFab::AddChatControlToNetwork(PartyLocalChatControl* LocalChatControl)
{
	if (Network && NetworkState == EPlayFabPartyNetworkState::NetworkReady)
	{
		PartyError Err = Network->ConnectChatControl(LocalChatControl, nullptr);

		if (PARTY_FAILED(Err))
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::AddChatControlToNetwork failed: %s\n"), *GetPartyErrorMessage(Err));
			return false;
		}

		return true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::AddChatControlToNetwork failed: Network is null or state is invalid!\n"))
	}

	return false;
}

bool FOnlineSubsystemPlayFab::InternalConnectToNetwork(PartyLocalUser* PlayFabPartyLocalUser, const FString& InNetworkId, Party::PartyNetworkDescriptor& InNetworkDescriptor)
{
	// This portion of connecting to the network is the same for
	// both creating a new and joining an existing network.

	if (PlayFabPartyLocalUser == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::InternalConnectToNetwork: Could not create new network PlayFabPartyLocalUser was null"));
		return false;
	}

	PartyError Err = PartyManager::GetSingleton().ConnectToNetwork(
		&InNetworkDescriptor,	// Network descriptor
		nullptr,				// Async identifier
		&Network				// OUT network
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::InternalConnectToNetwork: ConnectToNetwork failed: %s"), *GetPartyErrorMessage(Err));
		return false;
	}

	// Authenticate the local user on the network so we can participate in it
	Err = Network->AuthenticateLocalUser(
		PlayFabPartyLocalUser,			// Local user
		TCHAR_TO_UTF8(*InNetworkId),	// Invite value
		nullptr							// Async identifier
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::InternalConnectToNetwork: AuthenticateLocalUser failed: %s"), *GetPartyErrorMessage(Err));
		return false;
	}

	// Establish a network endpoint for game message traffic
	Err = Network->CreateEndpoint(
		PlayFabPartyLocalUser,	// Local user
		0,						// Property Count
		nullptr,				// Property name keys
		nullptr,				// Property Values
		nullptr,				// Async identifier
		&LocalEndpoint			// OUT local endpoint
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::InternalConnectToNetwork: CreateEndpoint failed: %s"), *GetPartyErrorMessage(Err));
		return false;
	}

	return true;
}

void FOnlineSubsystemPlayFab::LeavePlayFabPartyNetwork()
{
	UE_LOG_ONLINE(Log, TEXT("FOnlineSubsystemPlayFab::LeaveNetwork()"));

	if (NetworkState != EPlayFabPartyNetworkState::LeavingNetwork && Network)
	{
		NetworkState = EPlayFabPartyNetworkState::LeavingNetwork;
		Network->LeaveNetwork(nullptr);

		if (VoiceInterface)
		{
			VoiceInterface->OnLeavePlayFabPartyNetwork();
		}
	}
}

FString FOnlineSubsystemPlayFab::SerializeNetworkDescriptor(const PartyNetworkDescriptor& InNetworkDescriptor)
{
	FString NetworkDescriptorStr;
	
	char Descriptor[c_maxSerializedNetworkDescriptorStringLength + 1] = {};

	PartyError Err = PartyManager::SerializeNetworkDescriptor(
		&InNetworkDescriptor,
		Descriptor
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to serialize network descriptor: %s\n"), *GetPartyErrorMessage(Err));
	}
	else
	{
		NetworkDescriptorStr = ANSI_TO_TCHAR(Descriptor);
	}

	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::SerializeNetworkDescriptor: NetworkDescriptor: %s"), *NetworkDescriptorStr);

	return NetworkDescriptorStr;
}

FString GetPartyErrorMessage(PartyError InError)
{
	PartyString ErrorString = nullptr;

	PartyError Err = PartyManager::GetErrorMessage(InError, &ErrorString);
	if (PARTY_FAILED(Err))
	{
		return FString::Printf(TEXT("[ERROR] Failed to get error message %d.\n"), InError);
	}

	return ANSI_TO_TCHAR(ErrorString);
}

FString GetMultiplayerErrorMessage(HRESULT InHresult)
{
	const char* ErrorString = PFMultiplayerGetErrorMessage(InHresult);
	return ANSI_TO_TCHAR(ErrorString);
}

FString PartyStateChangeResultToReasonString(PartyStateChangeResult result)
{
	switch (result)
	{
	case PartyStateChangeResult::Succeeded:					 return TEXT("Succeeded");
	case PartyStateChangeResult::UnknownError:				 return TEXT("An unknown error occurred");
	case PartyStateChangeResult::InternetConnectivityError:  return TEXT("The local device has internet connectivity issues which caused the operation to fail");
	case PartyStateChangeResult::PartyServiceError:			 return TEXT("The CommunicationFabric service is unable to create a new network at this time");
	case PartyStateChangeResult::NoServersAvailable:		 return TEXT("There are no available servers in the regions specified by the call to PartyManager::CreateNewNetwork()");
	case PartyStateChangeResult::CanceledByTitle:			 return TEXT("Operation canceled by title.");
	case PartyStateChangeResult::UserCreateNetworkThrottled: return TEXT("The PartyLocalUser specified in the call to PartyManager::CreateNewNetwork() has created too many networks and cannot create new networks at this time");
	case PartyStateChangeResult::TitleNotEnabledForParty:	 return TEXT("The title has not been configured properly in the Party portal");
	case PartyStateChangeResult::NetworkLimitReached:		 return TEXT("The network is full and is not allowing new devices or users to join");
	case PartyStateChangeResult::NetworkNoLongerExists:		 return TEXT("The network no longer exists");
	case PartyStateChangeResult::NetworkNotJoinable:		 return TEXT("The network is not currently allowing new devices or users to join");
	case PartyStateChangeResult::VersionMismatch:			 return TEXT("The network uses a version of the CommunicationFabric library that is incompatible with this library");
	case PartyStateChangeResult::UserNotAuthorized:			 return TEXT("The specified user was not authorized");
	case PartyStateChangeResult::LeaveNetworkCalled:		 return TEXT("The network was gracefully exited by the local device");
	}
	return TEXT("Unknown enumeration value");
}

FString GetPartyStateChangeTypeString(PartyStateChangeType Type)
{
	switch (Type)
	{
	case PartyStateChangeType::RegionsChanged:									 return TEXT("RegionsChanged");
	case PartyStateChangeType::DestroyLocalUserCompleted:						 return TEXT("DestroyLocalUserCompleted");
	case PartyStateChangeType::CreateNewNetworkCompleted:						 return TEXT("CreateNewNetworkCompleted");
	case PartyStateChangeType::ConnectToNetworkCompleted:						 return TEXT("ConnectToNetworkCompleted");
	case PartyStateChangeType::AuthenticateLocalUserCompleted:					 return TEXT("AuthenticateLocalUserCompleted");
	case PartyStateChangeType::NetworkConfigurationMadeAvailable:				 return TEXT("NetworkConfigurationMadeAvailable");
	case PartyStateChangeType::NetworkDescriptorChanged:						 return TEXT("NetworkDescriptorChanged");
	case PartyStateChangeType::LocalUserRemoved:								 return TEXT("LocalUserRemoved");
	case PartyStateChangeType::RemoveLocalUserCompleted:						 return TEXT("RemoveLocalUserCompleted");
	case PartyStateChangeType::LocalUserKicked:									 return TEXT("LocalUserKicked");
	case PartyStateChangeType::CreateEndpointCompleted:							 return TEXT("CreateEndpointCompleted");
	case PartyStateChangeType::DestroyEndpointCompleted:						 return TEXT("DestroyEndpointCompleted");
	case PartyStateChangeType::EndpointCreated:									 return TEXT("EndpointCreated");
	case PartyStateChangeType::EndpointDestroyed:								 return TEXT("EndpointDestroyed");
	case PartyStateChangeType::RemoteDeviceCreated:								 return TEXT("RemoteDeviceCreated");
	case PartyStateChangeType::RemoteDeviceDestroyed:							 return TEXT("RemoteDeviceDestroyed");
	case PartyStateChangeType::RemoteDeviceJoinedNetwork:						 return TEXT("RemoteDeviceJoinedNetwork");
	case PartyStateChangeType::RemoteDeviceLeftNetwork:							 return TEXT("RemoteDeviceLeftNetwork");
	case PartyStateChangeType::DevicePropertiesChanged:							 return TEXT("DevicePropertiesChanged");
	case PartyStateChangeType::LeaveNetworkCompleted:							 return TEXT("LeaveNetworkCompleted");
	case PartyStateChangeType::NetworkDestroyed:								 return TEXT("NetworkDestroyed");
	case PartyStateChangeType::EndpointMessageReceived:							 return TEXT("EndpointMessageReceived");
	case PartyStateChangeType::DataBuffersReturned:								 return TEXT("DataBuffersReturned");
	case PartyStateChangeType::EndpointPropertiesChanged:						 return TEXT("EndpointPropertiesChanged");
	case PartyStateChangeType::SynchronizeMessagesBetweenEndpointsCompleted:	 return TEXT("SynchronizeMessagesBetweenEndpointsCompleted");
	case PartyStateChangeType::CreateInvitationCompleted:						 return TEXT("CreateInvitationCompleted");
	case PartyStateChangeType::RevokeInvitationCompleted:						 return TEXT("RevokeInvitationCompleted");
	case PartyStateChangeType::InvitationCreated:								 return TEXT("InvitationCreated");
	case PartyStateChangeType::InvitationDestroyed:								 return TEXT("InvitationDestroyed");
	case PartyStateChangeType::NetworkPropertiesChanged:						 return TEXT("NetworkPropertiesChanged");
	case PartyStateChangeType::KickDeviceCompleted:								 return TEXT("KickDeviceCompleted");
	case PartyStateChangeType::KickUserCompleted:								 return TEXT("KickUserCompleted");
	case PartyStateChangeType::SetChatAudioEncoderBitrateCompleted:				 return TEXT("SetChatAudioEncoderBitrateCompleted");
	case PartyStateChangeType::LocalChatAudioOutputChanged:						 return TEXT("LocalChatAudioOutputChanged");
	case PartyStateChangeType::SetLanguageCompleted:							 return TEXT("SetLanguageCompleted");
	case PartyStateChangeType::ChatControlPropertiesChanged:					 return TEXT("ChatControlPropertiesChanged");
	case PartyStateChangeType::DisconnectChatControlCompleted:					 return TEXT("DisconnectChatControlCompleted");
	case PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted:	 return TEXT("PopulateAvailableTextToSpeechProfilesCompleted");
	case PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted:	 return TEXT("ConfigureAudioManipulationVoiceStreamCompleted");
	case PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted: return TEXT("ConfigureAudioManipulationCaptureStreamCompleted");
	case PartyStateChangeType::ConfigureAudioManipulationRenderStreamCompleted:	 return TEXT("ConfigureAudioManipulationRenderStreamCompleted");
	case PartyStateChangeType::ChatTextReceived:								 return TEXT("ChatTextReceived");
	case PartyStateChangeType::VoiceChatTranscriptionReceived:					 return TEXT("VoiceChatTranscriptionReceived");
	case PartyStateChangeType::SetTextToSpeechProfileCompleted:					 return TEXT("SetTextToSpeechProfileCompleted");
	case PartyStateChangeType::SynthesizeTextToSpeechCompleted:					 return TEXT("SynthesizeTextToSpeechCompleted");
	case PartyStateChangeType::SetTranscriptionOptionsCompleted:				 return TEXT("SetTranscriptionOptionsCompleted");
	case PartyStateChangeType::SetTextChatOptionsCompleted:						 return TEXT("SetTextChatOptionsCompleted");
	}

	return TEXT("Unknown PartyStateChangeType");
}

FString GetNetworkStateStateString(EPlayFabPartyNetworkState State)
{
	switch (State)
	{
	case EPlayFabPartyNetworkState::NoNetwork: return TEXT("NoNetwork");
	case EPlayFabPartyNetworkState::JoiningNetwork_Host: return TEXT("JoiningNetwork_Host");
	case EPlayFabPartyNetworkState::JoiningNetwork_Host_PendingEndpointCreation: return TEXT("JoiningNetwork_Host_PendingEndpointCreation");
	case EPlayFabPartyNetworkState::JoiningNetwork_Client: return TEXT("JoiningNetwork_Client");
	case EPlayFabPartyNetworkState::NetworkReady: return TEXT("NetworkReady");
	case EPlayFabPartyNetworkState::LeavingNetwork: return TEXT("LeavingNetwork");
	}
	return TEXT("Unknown");
}

void FOnlineSubsystemPlayFab::OnCreateNewNetworkCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnCreateNewNetworkCompleted"));

	const PartyCreateNewNetworkCompletedStateChange* Result = static_cast<const PartyCreateNewNetworkCompletedStateChange*>(Change);
	if (Result)
	{
		if (Result->result == PartyStateChangeResult::Succeeded)
		{
			UE_LOG_ONLINE(Verbose, TEXT("CreateNewNetworkCompleted: SUCCESS"));
			if (Result->localUser)
			{
				PartyString EntityId;
				Result->localUser->GetEntityId(&EntityId);

				UE_LOG_ONLINE(Verbose, TEXT("CreateNewNetworkCompleted: EntityId: %s"), ANSI_TO_TCHAR(EntityId));
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("CreateNewNetworkCompleted: FAIL: %s"), *PartyStateChangeResultToReasonString(Result->result));
			UE_LOG_ONLINE(Warning, TEXT("ErrorDetail: %s"), *GetPartyErrorMessage(Result->errorDetail));
		}
	}
}

void FOnlineSubsystemPlayFab::OnConnectToNetworkCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnConnectToNetworkCompleted"));

	const PartyConnectToNetworkCompletedStateChange* Result = static_cast<const PartyConnectToNetworkCompletedStateChange*>(Change);
	if (Result)
	{
		if (Result->result == PartyStateChangeResult::Succeeded)
		{
			UE_LOG_ONLINE(Verbose, TEXT("ConnectToNetworkCompleted: SUCCESS"));

			// We branch here if we are the host, since we need to wait for the local endpoint to be created to store the info in the session / have a valid 'PlayFab IP addr' class instance
			if (NetworkState == EPlayFabPartyNetworkState::JoiningNetwork_Host)
			{
				NetworkState = EPlayFabPartyNetworkState::JoiningNetwork_Host_PendingEndpointCreation;
			}
			else
			{
				NetworkState = EPlayFabPartyNetworkState::NetworkReady;
			}
			NetworkDescriptor = Result->networkDescriptor;

			TriggerOnConnectToPlayFabPartyNetworkCompletedDelegates(true);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("ConnectToNetworkCompleted: FAIL:  %s"), *PartyStateChangeResultToReasonString(Result->result));
			UE_LOG_ONLINE(Warning, TEXT("ErrorDetail: %s"), *GetPartyErrorMessage(Result->errorDetail));

			TriggerOnConnectToPlayFabPartyNetworkCompletedDelegates(false);
		}
	}
}

void FOnlineSubsystemPlayFab::OnLeaveNetworkCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnLeaveNetworkCompleted"));

	const PartyLeaveNetworkCompletedStateChange* Result = static_cast<const PartyLeaveNetworkCompletedStateChange*>(Change);
	if (Result)
	{
		NetworkState = EPlayFabPartyNetworkState::NoNetwork;
		Network = nullptr;
	}
}

void FOnlineSubsystemPlayFab::OnNetworkDestroyed(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnNetworkDestroyed"));

	const PartyNetworkDestroyedStateChange* Result = static_cast<const PartyNetworkDestroyedStateChange*>(Change);
	if (Result)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::OnNetworkDestroyed: PlayFab Party network was destroyed with reason code %d"), Result->reason);

		FPlayFabSocketSubsystem* SocketSubsystem = static_cast<FPlayFabSocketSubsystem*>(ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM));
		if (SocketSubsystem == nullptr)
		{
			return;
		}

		UPlayFabNetDriver* PlayFabNetDriver = SocketSubsystem->GetLinkedNetDriver();
		if (PlayFabNetDriver == nullptr)
		{
			return;
		}

		UIpConnection* const ServerConnection = PlayFabNetDriver->GetServerConnection();

		if (ServerConnection != nullptr)
		{
			ServerConnection->Close();
		}
		else // We are the host and must handle this, UE won't handle it like a client because the host connection wont get closed
		{
			// was it an error? If so what kind
			FString ErrorString;
			if (Result->reason == PartyDestroyedReason::Requested)
			{
				// not an error
			}
			else if (Result->reason == PartyDestroyedReason::Disconnected)
			{
				ErrorString = FString::Printf(TEXT("You have lost connection to the multiplayer service."));
			}
			else if (Result->reason == PartyDestroyedReason::Kicked)
			{
				ErrorString = FString::Printf(TEXT("You were kicked from the match."));
			}
			else if (Result->reason == PartyDestroyedReason::DeviceLostAuthentication)
			{
				ErrorString = FString::Printf(TEXT("You have lost connection to the authentication service."));
			}
			else if (Result->reason == PartyDestroyedReason::CreationFailed)
			{
				ErrorString = FString::Printf(TEXT("Failed to create a match."));
			}

			if (!ErrorString.IsEmpty())
			{
				GEngine->BroadcastNetworkFailure(PlayFabNetDriver->GetWorld(), PlayFabNetDriver, ENetworkFailure::PendingConnectionFailure, ErrorString);
			}
		}
	
	}
}

void FOnlineSubsystemPlayFab::OnEndpointMessageReceived(const PartyStateChange* Change)
{
	const PartyEndpointMessageReceivedStateChange* Result = static_cast<const PartyEndpointMessageReceivedStateChange*>(Change);
	if (Result)
	{
		TriggerOnEndpointMessageReceivedDelegates(Result);
	}
}

void FOnlineSubsystemPlayFab::OnNetworkConfigurationMadeAvailable(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnNetworkConfigurationMadeAvailable"));

	const PartyNetworkConfigurationMadeAvailableStateChange* Result = static_cast<const PartyNetworkConfigurationMadeAvailableStateChange*>(Change);
	if (Result)
	{
		if (Result->networkConfiguration)
		{
			MaxDeviceCount = Result->networkConfiguration->maxDeviceCount;
			MaxDevicesPerUserCount = Result->networkConfiguration->maxDevicesPerUserCount;
			MaxEndpointsPerDeviceCount = Result->networkConfiguration->maxEndpointsPerDeviceCount;
			MaxUserCount = Result->networkConfiguration->maxUserCount;
			MaxUsersPerDeviceCount = Result->networkConfiguration->maxUsersPerDeviceCount;
			DirectPeerConnectivityOptions = Result->networkConfiguration->directPeerConnectivityOptions;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::OnNetworkConfigurationMadeAvailable: The NetworkConfiguration returned was null"));
		}
	}
}

void FOnlineSubsystemPlayFab::OnNetworkDescriptorChanged(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnNetworkDescriptorChanged"));

	const PartyNetworkDescriptorChangedStateChange* Result = static_cast<const PartyNetworkDescriptorChangedStateChange*>(Change);
	if (Result)
	{
		if (Result->network)
		{
			PartyNetworkDescriptor* NewNetorkDescriptor = nullptr;

			PartyError Err = Result->network->GetNetworkDescriptor(NewNetorkDescriptor);

			if (PARTY_FAILED(Err))
			{
				UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::OnNetworkDescriptorChanged: Failed to get network descriptor from network: %s"), *GetPartyErrorMessage(Err));
				return;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::OnNetworkDescriptorChanged: The PartyNetwork returned was null"));
		}
	}
}

void FOnlineSubsystemPlayFab::OnNetworkPropertiesChanged(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnNetworkPropertiesChanged"));
}

void FOnlineSubsystemPlayFab::OnCreateEndpointCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnCreateEndpointCompleted"));
}

void FOnlineSubsystemPlayFab::OnDestroyEndpointCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnDestroyEndpointCompleted"));
}

void FOnlineSubsystemPlayFab::OnEndpointCreated(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnEndpointCreated"));

	const PartyEndpointCreatedStateChange* Result = static_cast<const PartyEndpointCreatedStateChange*>(Change);
	if (Result)
	{
		bool bIsHosting = NetworkState == EPlayFabPartyNetworkState::JoiningNetwork_Host_PendingEndpointCreation;

		PartyEndpoint* NewEndpoint = Result->endpoint;
		if (NewEndpoint)
		{
			uint16 EndpointId = 0;
			PartyError Err = NewEndpoint->GetUniqueIdentifier(&EndpointId);
			if (PARTY_FAILED(Err) || EndpointId == 0)
			{
				UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::OnEndpointCreated: Unable to retrieve UniqueIdentifier from endpoint: %s"), *GetPartyErrorMessage(Err));

				

				TriggerOnPartyEndpointCreatedDelegates(false, 0, bIsHosting);
			}
			else
			{
				Endpoints.Add(EndpointId, NewEndpoint);
				NetworkState = EPlayFabPartyNetworkState::NetworkReady;
				TriggerOnPartyEndpointCreatedDelegates(true, EndpointId, bIsHosting);

				UE_LOG(LogNet, Warning, TEXT("FOnlineSubsystemPlayFab::OnEndpointCreate: Created Party Endpoint: %d"), EndpointId);
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::OnEndpointCreated: returned enpoint was invalid"));
			TriggerOnPartyEndpointCreatedDelegates(false, 0, bIsHosting);
		}
	}
}

void FOnlineSubsystemPlayFab::OnEndpointDestroyed(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnEndpointDestroyed"));

	const PartyEndpointDestroyedStateChange* Result = static_cast<const PartyEndpointDestroyedStateChange*>(Change);
	if (Result)
	{
		PartyEndpoint* NewEndpoint = Result->endpoint;
		if (NewEndpoint)
		{
			uint16 EndpointId = 0;
			PartyError Err = NewEndpoint->GetUniqueIdentifier(&EndpointId);
			if (PARTY_FAILED(Err) || EndpointId == 0)
			{
				UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::OnEndpointDestroyed: Unable to retrieve UniqueIdentifier from endpoint: %s"), *GetPartyErrorMessage(Err));
			}
			else
			{
				Endpoints.Remove(EndpointId);
			}

			UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnEndpointDestroyed: Destroying endpoint %d with reason code %d"), EndpointId, Result->reason);

			// Also close the net connection for instantaneous disconnection rather than a timeout
			FPlayFabSocketSubsystem* SocketSubsystem = static_cast<FPlayFabSocketSubsystem*>(ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM));
			if (SocketSubsystem == nullptr)
			{
				return;
			}

			UPlayFabNetDriver* PlayFabNetDriver = SocketSubsystem->GetLinkedNetDriver();
			if (PlayFabNetDriver == nullptr)
			{
				return;
			}

			// Was it the server connection? Also handled in network destroy but lets close for sanity
			UIpConnection* const ServerConnection = PlayFabNetDriver->GetServerConnection();
			UNetConnection* ConnectionToClose = nullptr;
			if (ServerConnection != nullptr && ServerConnection->GetAddrPort() == EndpointId)
			{
				ConnectionToClose = ServerConnection;
			}
			else
			{
				TSharedRef<FInternetAddr> CompareAddr = SocketSubsystem->CreateInternetAddr();
				CompareAddr->SetIp(EndpointId);
				CompareAddr->SetPort(EndpointId);
				UNetConnection** ConnectionFound = PlayFabNetDriver->MappedClientConnections.Find(CompareAddr);
				if (ConnectionFound != nullptr)
				{
					ConnectionToClose = *ConnectionFound;
				}
			}

			if (ConnectionToClose != nullptr)
			{
				ConnectionToClose->Close();
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::OnEndpointDestroyed: returned enpoint was invalid"));
		}
	}
}

void FOnlineSubsystemPlayFab::OnEndpointPropertiesChanged(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnEndpointPropertiesChanged"));
}

void FOnlineSubsystemPlayFab::OnRegionsChanged(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnRegionsChanged"));
}

void FOnlineSubsystemPlayFab::OnDestroyLocalUserCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnDestroyLocalUserCompleted"));
}

void FOnlineSubsystemPlayFab::OnAuthenticateLocalUserCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnAuthenticateLocalUserCompleted"));
}

void FOnlineSubsystemPlayFab::OnRemoteDeviceDestroyed(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnRemoteDeviceDestroyed"));
}

void FOnlineSubsystemPlayFab::OnRemoteDeviceJoinedNetwork(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnRemoteDeviceJoinedNetwork"));
}

void FOnlineSubsystemPlayFab::OnRemoteDeviceLeftNetwork(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnRemoteDeviceLeftNetwork"));
}

void FOnlineSubsystemPlayFab::OnDevicePropertiesChanged(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnDevicePropertiesChanged"));
}

void FOnlineSubsystemPlayFab::OnDataBuffersReturned(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnDataBuffersReturned"));
}

void FOnlineSubsystemPlayFab::OnLocalUserRemoved(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnLocalUserRemoved"));
}

void FOnlineSubsystemPlayFab::OnRemoveLocalUserCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnRemoveLocalUserCompleted"));
}

void FOnlineSubsystemPlayFab::OnLocalUserKicked(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnLocalUserKicked"));
}

void FOnlineSubsystemPlayFab::OnCreateInvitationCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnCreateInvitationCompleted"));
}

void FOnlineSubsystemPlayFab::OnRevokeInvitationCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnRevokeInvitationCompleted"));
}

void FOnlineSubsystemPlayFab::OnInvitationCreated(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnInvitationCreated"));
}

void FOnlineSubsystemPlayFab::OnInvitationDestroyed(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnInvitationDestroyed"));
}

void FOnlineSubsystemPlayFab::OnKickDeviceCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnKickDeviceCompleted"));
}

void FOnlineSubsystemPlayFab::OnKickUserCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnKickUserCompleted"));
}

void FOnlineSubsystemPlayFab::OnRemoteDeviceCreated(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnRemoteDeviceCreated"));
}

void FOnlineSubsystemPlayFab::OnSynchronizeMessagesBetweenEndpointsCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnSynchronizeMessagesBetweenEndpointsCompleted"));
}

void FOnlineSubsystemPlayFab::OnSetChatAudioEncoderBitrateCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnSetChatAudioEncoderBitrateCompleted"));
}

void FOnlineSubsystemPlayFab::OnLocalChatAudioOutputChanged(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnLocalChatAudioOutputChanged"));
}

void FOnlineSubsystemPlayFab::OnSetLanguageCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnSetLanguageCompleted"));
}

void FOnlineSubsystemPlayFab::OnChatControlPropertiesChanged(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnChatControlPropertiesChanged"));
}

void FOnlineSubsystemPlayFab::OnDisconnectChatControlCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnDisconnectChatControlCompleted"));
}

void FOnlineSubsystemPlayFab::OnConfigureAudioManipulationVoiceStreamCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnConfigureAudioManipulationVoiceStreamCompleted"));
}

void FOnlineSubsystemPlayFab::OnConfigureAudioManipulationCaptureStreamCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnConfigureAudioManipulationCaptureStreamCompleted"));
}

void FOnlineSubsystemPlayFab::OnConfigureAudioManipulationRenderStreamCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSubsystemPlayFab::OnConfigureAudioManipulationRenderStreamCompleted"));
}

PartyEndpoint* FOnlineSubsystemPlayFab::GetPartyEndpoint(uint32 EndpointId)
{
	if (EndpointId != 0)
	{
		PartyEndpoint** PlayFabPartyEndpoint = Endpoints.Find(EndpointId);

		if (PlayFabPartyEndpoint == nullptr || *PlayFabPartyEndpoint == nullptr)
		{
			UE_LOG(LogSockets, Warning, TEXT("FOnlineSubsystemPlayFab::GetPartyEndpoint: Could not find Enpoint for UniqueId %d"), EndpointId);
		}
		else
		{
			return *PlayFabPartyEndpoint;
		}
	}
	else
	{
		UE_LOG(LogSockets, Warning, TEXT("FOnlineSubsystemPlayFab::GetPartyEndpoint: UniqueId was invalid"));
	}

	return nullptr;
}

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
const FString FOnlineSubsystemPlayFab::GetSandBox() const
{
	char SandboxId[XSystemXboxLiveSandboxIdMaxBytes] = { 0 };
	HRESULT hResult = XSystemGetXboxLiveSandboxId(UE_ARRAY_COUNT(SandboxId), SandboxId, nullptr);
	if (SUCCEEDED(hResult))
	{
		return UTF8_TO_TCHAR(SandboxId);
	}
	UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemPlayFab::GetSandBox: failed: %x"), hResult);
	return TEXT("");
}
#endif // OSS_PLAYFAB_WINGDK || OSS_PLAYFAB_XSX || OSS_PLAYFAB_XBOXONEGDK