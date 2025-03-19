//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSessionSettings.h"
#include "PlayFabSocketSubsystem.h"
#include "../PlatformSpecific/PlatformDefines.h"

#include "OnlineSubsystemPlayFab.h"
#include "OnlineSubsystemPlayFabPrivate.h"

#include "GenericPlatform/GenericPlatformHttp.h"

#define OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();  IOnlineSessionPtr NativeSessionInterface = NativeSubsystem ? NativeSubsystem->GetSessionInterface() : nullptr; if (NativeSessionInterface)

FOnlineSessionPlayFab::FOnlineSessionPlayFab(class FOnlineSubsystemPlayFab* InSubsystem) :
	OSSPlayFab(InSubsystem)
{
	check(OSSPlayFab);

#if defined(USES_NATIVE_SESSION)
	bUsesNativeSession = true;
#endif
#if defined(OSS_PLAYFAB_PLAYSTATION)
	UpdateNativeSessionName();
#endif
#if defined(OSS_PLAYFAB_GDK)
	RegisterForInvites();
#endif
	RegisterForUpdates();
	GenerateCrossNetworkVoiceChatPlatformPermissions();
}

FOnlineSessionPlayFab::~FOnlineSessionPlayFab()
{
#if defined(OSS_PLAYFAB_GDK)
	UnregisterForInvites();
#endif

	// Clean up delegates
	OSSPlayFab->ClearOnConnectToPlayFabPartyNetworkCompletedDelegates(this);
	ClearOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegateHandle);
	UnregisterForUpdates();
}

TSharedPtr<const FUniqueNetId> FOnlineSessionPlayFab::CreateSessionIdFromString(const FString& SessionIdStr)
{
	if (!SessionIdStr.IsEmpty())
	{
		return FUniqueNetIdPlayFab::Create(SessionIdStr);
	}
	return nullptr;
}

FNamedOnlineSession* FOnlineSessionPlayFab::GetNamedSession(FName SessionName)
{
	FScopeLock ScopeLock(&SessionLock);
	for (FNamedOnlineSessionRef& Session : Sessions)
	{
		if (Session->SessionName == SessionName)
		{
			return &Session.Get();
		}
	}

	return nullptr;
}

void FOnlineSessionPlayFab::RemoveNamedSession(FName SessionName)
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex]->SessionName == SessionName)
		{
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RemoveNamedSession: Removing Session %s"), *SessionName.ToString());
			Sessions.RemoveAtSwap(SearchIndex);
			return;
		}
	}
}

bool FOnlineSessionPlayFab::HasPresenceSession()
{
	FScopeLock ScopeLock(&SessionLock);
	for (const FNamedOnlineSessionRef& Session : Sessions)
	{
		if (Session->SessionSettings.bUsesPresence)
		{
			return true;
		}
	}

	return false;
}

EOnlineSessionState::Type FOnlineSessionPlayFab::GetSessionState(FName SessionName) const
{
	FScopeLock ScopeLock(&SessionLock);
	for (const FNamedOnlineSessionRef& Session : Sessions)
	{
		if (Session->SessionName == SessionName)
		{
			return Session->SessionState;
		}
	}

	return EOnlineSessionState::NoSession;
}

bool FOnlineSessionPlayFab::CreateSession(int32 HostingPlayerControllerIndex, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::CreateSession()"));

	bool bSuccess = false;
	
	if (NewSessionSettings.bIsLANMatch)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::CreateSession: OnlineSubsystemPlayFab doe not support LAN sessions"));
		OnCreateSessionCompleted(SessionName, false);
		return false;
	}
	
	// Check for an existing session
	if (GetNamedSessionPtr(SessionName).IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::CreateSession: Cannot create session '%s': session already exists."), *SessionName.ToString());
		OnCreateSessionCompleted(SessionName, false);
		return false;
	}

	PendingCreateSessionInfo.PlayerControllerIndex = HostingPlayerControllerIndex;
	auto allUsers = OSSPlayFab->GetIdentityInterface()->GetAllUserAccounts();
	if (allUsers.Num() > HostingPlayerControllerIndex)
	{
		PendingCreateSessionInfo.PlayerId = allUsers[HostingPlayerControllerIndex]->GetUserId()->AsShared();
	}
	else
	{
		PendingCreateSessionInfo.PlayerId = nullptr;
	}
	PendingCreateSessionInfo.SessionName = SessionName;
#if !defined(OSS_PLAYFAB_PLAYSTATION)
	NativeSessionName = SessionName;
#endif
	PendingCreateSessionInfo.SessionSettings = NewSessionSettings;

	OSSPlayFab->AddOnPartyEndpointCreatedDelegate_Handle(FOnPartyEndpointCreatedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCreatePartyEndpoint));

	bSuccess = OSSPlayFab->CreateAndConnectToPlayFabPartyNetwork();

	if (bSuccess == false)
	{
		OnCreateSessionCompleted(SessionName, false);
	}

	return bSuccess;
}

bool FOnlineSessionPlayFab::CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::CreateSession()"));

	bool bSuccess = false;

	if (NewSessionSettings.bIsLANMatch)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::CreateSession: OnlineSubsystemPlayFab doe not support LAN sessions"));
		OnCreateSessionCompleted(SessionName, false);
		return false;
	}

	// Check for an existing session
	if (GetNamedSessionPtr(SessionName).IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::CreateSession: Cannot create session '%s': session already exists."), *SessionName.ToString());
		OnCreateSessionCompleted(SessionName, false);
		return false;
	}

	PendingCreateSessionInfo.PlayerControllerIndex = INDEX_NONE;
	PendingCreateSessionInfo.PlayerId = HostingPlayerId.AsShared();
	PendingCreateSessionInfo.SessionName = SessionName;
#if !defined(OSS_PLAYFAB_PLAYSTATION)
	NativeSessionName = SessionName;
#endif
	PendingCreateSessionInfo.SessionSettings = NewSessionSettings;

	OSSPlayFab->AddOnPartyEndpointCreatedDelegate_Handle(FOnPartyEndpointCreatedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCreatePartyEndpoint));
	bSuccess = OSSPlayFab->CreateAndConnectToPlayFabPartyNetwork();

	if (bSuccess == false)
	{
		OnCreateSessionCompleted(SessionName, false);
	}

	return bSuccess;
}

void FOnlineSessionPlayFab::OnCreatePartyEndpoint(bool bSuccess, uint16 EndpointID, bool bIsHosting)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint()"));

	OSSPlayFab->ClearOnPartyEndpointCreatedDelegates(this);

	if (bIsHosting)
	{
		ISocketSubsystem* PlayFabSocketSubsystem = ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM);
		if (bSuccess && OSSPlayFab && PlayFabSocketSubsystem)
		{
			bool BindAll = false;
			TSharedRef<FInternetAddr> LocalIp = PlayFabSocketSubsystem->GetLocalHostAddr(*GLog, BindAll);
			if (LocalIp->IsValid())
			{
				FString NetworkIdStr = OSSPlayFab->NetworkId;
				FString NetworkDescriptorStr = OSSPlayFab->SerializeNetworkDescriptor(OSSPlayFab->NetworkDescriptor);
				FString HostConnectInfo = LocalIp->ToString(false);

				PendingCreateSessionInfo.SessionSettings.Set(SETTING_NETWORK_ID, NetworkIdStr, EOnlineDataAdvertisementType::ViaOnlineService);
				PendingCreateSessionInfo.SessionSettings.Set(SETTING_NETWORK_DESCRIPTOR, NetworkDescriptorStr, EOnlineDataAdvertisementType::ViaOnlineService);
				PendingCreateSessionInfo.SessionSettings.Set(SETTING_HOST_CONNECT_INFO, HostConnectInfo, EOnlineDataAdvertisementType::ViaOnlineService);
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint: LocalHostAddr was invalid"));
			}

			if (!(InternalCreateSession(*PendingCreateSessionInfo.PlayerId, PendingCreateSessionInfo.SessionName, PendingCreateSessionInfo.SessionSettings)))
			{
				OnCreateSessionCompleted(PendingCreateSessionInfo.SessionName, false);
			}
		}
		else
		{
			OnCreateSessionCompleted(PendingCreateSessionInfo.SessionName, false);
		}
	}
}

bool FOnlineSessionPlayFab::InternalCreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::InternalCreateSession()"));

	TSharedRef<FOnlineSessionInfoPlayFab> NewSessionInfo = MakeShared<FOnlineSessionInfoPlayFab>();
	NewSessionInfo->SessionName = SessionName;

	// Create a new session and deep copy the game settings
	FNamedOnlineSessionRef Session = AddNamedSessionRef(SessionName, NewSessionSettings);
	Session->SessionState = EOnlineSessionState::Creating;
	Session->bHosting = true;
	Session->OwningUserId = FUniqueNetIdPlayFab::Create(HostingPlayerId);
	Session->LocalOwnerId = HostingPlayerId.AsShared();
	Session->SessionInfo = NewSessionInfo;

	// Unique identifier of this build for compatibility
	Session->SessionSettings.BuildUniqueId = GetBuildUniqueId();

	if (!OSSPlayFab)
	{
		return false;
	}

	OnLobbyCreatedAndJoinCompletedDelegateHandle = FOnLobbyCreatedAndJoinCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyCreatedAndJoinCompleted);
	OnLobbyCreatedAndJoinCompletedHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyCreatedAndJoinCompletedDelegate_Handle(OnLobbyCreatedAndJoinCompletedDelegateHandle);

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->CreatePlayFabLobby(HostingPlayerId, SessionName, NewSessionSettings))
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::InternalCreateSession: Failed to create create lobby operation"));
		RemoveNamedSession(SessionName);
		return false;
	}

	return true;
}

void FOnlineSessionPlayFab::OnLobbyCreatedAndJoinCompleted(bool fSuccess, FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyCreatedAndJoinCompleted()"));

	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyCreatedAndJoinCompletedDelegate_Handle(OnLobbyCreatedAndJoinCompletedHandle);
	OnCreateSessionCompleted(SessionName, fSuccess);
}

bool FOnlineSessionPlayFab::StartSession(FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::StartSession()"));

	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	if (!Session.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::StartSession: Can't start an online game for session (%s) that hasn't been created"), *SessionName.ToString());
		TriggerOnStartSessionCompleteDelegates(SessionName, false);
		return false;
	}

	if (Session->SessionState != EOnlineSessionState::Pending &&
		Session->SessionState != EOnlineSessionState::Ended)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::StartSession: Can't start an online session (%s) in state %s"), *SessionName.ToString(), EOnlineSessionState::ToString(Session->SessionState));
		TriggerOnStartSessionCompleteDelegates(SessionName, false);
		return false;
	}

	Session->SessionState = EOnlineSessionState::InProgress;

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			if (NativeSessionInterface->StartSession(NativeSessionName))
			{
				TriggerOnStartSessionCompleteDelegates(SessionName, true);
				return true;
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::StartSession: Start native session (%s) failed"), *NativeSessionName.ToString());
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::StartSession: Failed to get native session (%s) interface"), *NativeSessionName.ToString());
		}
	
		TriggerOnStartSessionCompleteDelegates(SessionName, false);
		return false;
	}

	TriggerOnStartSessionCompleteDelegates(SessionName, true);
	return true;
}

bool FOnlineSessionPlayFab::UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData /*= false*/)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UpdateSession()"));

	FNamedOnlineSessionPtr NamedSession = GetNamedSessionPtr(SessionName);
	if (!NamedSession.IsValid())
	{
		OSSPlayFab->ExecuteNextTick([this, SessionName]()
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::UpdateSession: Unable to update session %s, it does not exist"), *SessionName.ToString());
			TriggerOnUpdateSessionCompleteDelegates(SessionName, false);
		});
		return false;
	}

	EOnlineSessionState::Type SessionState = NamedSession->SessionState;
	if (SessionState <= EOnlineSessionState::Creating || SessionState >= EOnlineSessionState::Destroying)
	{
		OSSPlayFab->ExecuteNextTick([this, SessionName, SessionState]()
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::UpdateSession: Unable to update session %s, it is state %s, which may not be updated"), *SessionName.ToString(), EOnlineSessionState::ToString(SessionState));
			TriggerOnUpdateSessionCompleteDelegates(SessionName, false);
		});
		return false;
	}

	if (OnUpdateLobbyCompleteDelegate.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::UpdateSession: UpdateSession is already in progress. Call UpdateSession again after the current operation is completed."));
		return false;
	}

	OnUpdateLobbyCompleteDelegate = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnUpdateLobbyCompletedDelegate_Handle(FOnUpdateLobbyCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnUpdateLobbyCompleted));
	if (!OSSPlayFab->GetPlayFabLobbyInterface()->UpdateLobby(SessionName, UpdatedSessionSettings))
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UpdateSession: Failed to update session %s"), *SessionName.ToString());
		OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnUpdateLobbyCompletedDelegate_Handle(OnUpdateLobbyCompleteDelegate);
		return false;
	}

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			FOnlineSessionSettings SessionSettings = UpdatedSessionSettings;
			SessionSettings.Set(SETTING_CONNECTION_STRING, ConnectionString, EOnlineDataAdvertisementType::ViaOnlineService);
			OnNativeUpdateSessionCompleteDelegateHandle = NativeSessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(
				FOnUpdateSessionCompleteDelegate::CreateLambda([this, NativeSessionInterface](FName SessionName, bool bNativeSessionUpdated)
					{
						if (bNativeSessionUpdated == false)
						{
							UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeUpdateSessionComplete: Failed to update native session"));
						}

						NativeSessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(OnNativeUpdateSessionCompleteDelegateHandle);
					}));
			return NativeSessionInterface->UpdateSession(NativeSessionName, SessionSettings, bShouldRefreshOnlineData);
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UpdateSession: Failed to to get native session inteface %s"), *NativeSessionName.ToString());
			return false;
		}
	}

	return true;
}

bool FOnlineSessionPlayFab::EndSession(FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::EndSession()"));

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			ConnectionString = TEXT("");
			NativeSessionInterface->EndSession(NativeSessionName);
		}
	}

	// Reset bUsesNativeSession to true for platforms using native session interface
#if defined(USES_NATIVE_SESSION)
	bUsesNativeSession = true;
#endif
	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	if (!Session.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::EndSession: Can't end an online game for session (%s) that hasn't been created"), *SessionName.ToString());
		TriggerOnEndSessionCompleteDelegates(SessionName, false);
		return false;
	}

	if (Session->SessionState != EOnlineSessionState::InProgress)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::EndSession: Can't end session (%s) in state %s"), *SessionName.ToString(), EOnlineSessionState::ToString(Session->SessionState));
		TriggerOnEndSessionCompleteDelegates(SessionName, false);
		return false;
	}

	Session->SessionState = EOnlineSessionState::Ended;
	TriggerOnEndSessionCompleteDelegates(SessionName, true);
	return true;
}

bool FOnlineSessionPlayFab::DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate /*= FOnDestroySessionCompleteDelegate()*/)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::DestroySession: SessionName:%s"), *SessionName.ToString());

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			ConnectionString = TEXT("");
			NativeSessionInterface->DestroySession(NativeSessionName);
		}
	}

	// Leave the PlayFab Party network
	OSSPlayFab->LeavePlayFabPartyNetwork();

	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	if (!Session.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::DestroySession: Can't destroy a null online session (%s)"), *SessionName.ToString());
		OSSPlayFab->ExecuteNextTick([this, SessionName, CompletionDelegate]()
		{
			CompletionDelegate.ExecuteIfBound(SessionName, false);
			TriggerOnDestroySessionCompleteDelegates(SessionName, false);
		});
		return false;
	}

	if (Session->SessionState == EOnlineSessionState::Destroying)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::DestroySession: Already in process of destroying session (%s)"), *SessionName.ToString());
		OSSPlayFab->ExecuteNextTick([this, SessionName, CompletionDelegate]()
		{
			CompletionDelegate.ExecuteIfBound(SessionName, false);
			TriggerOnDestroySessionCompleteDelegates(SessionName, false);
		});
		return false;
	}

	Session->SessionState = EOnlineSessionState::Destroying;

	OnLeaveLobbyCompletedHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLeaveLobbyCompletedDelegate_Handle(FOnLeaveLobbyCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLeaveLobbyCompleted));

	// TODO GDK OSS will leave the session in next tick? should PF OSS also use ExecuteNextTick?
	if (!OSSPlayFab->GetPlayFabLobbyInterface()->LeaveLobby(*FUniqueNetIdPlayFab::EmptyId(), SessionName, CompletionDelegate, FOnUnregisterLocalPlayerCompleteDelegate(), true))
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::DestroySession: Failed to destroy the session %s"), *SessionName.ToString());
		OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLeaveLobbyCompletedDelegate_Handle(OnLeaveLobbyCompletedHandle);

		OSSPlayFab->ExecuteNextTick([this, SessionName, CompletionDelegate]()
			{
				CompletionDelegate.ExecuteIfBound(SessionName, false);
				TriggerOnDestroySessionCompleteDelegates(SessionName, false);
			});
		return false;
	}

	return true;
}

void FOnlineSessionPlayFab::OnLeaveLobbyCompleted(FName SessionName, bool bSuccess)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLeaveLobbyCompleted()"));

	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLeaveLobbyCompletedDelegate_Handle(OnLeaveLobbyCompletedHandle);
	TriggerOnDestroySessionCompleteDelegates(SessionName, bSuccess);
}

void FOnlineSessionPlayFab::OnFindLobbiesCompleted(int32 LocalUserNum, bool bSuccess, TSharedPtr<FOnlineSessionSearch> SearchResults)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnFindLobbiesCompleted()"));

	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnFindLobbiesCompletedDelegate_Handle(OnFindLobbiesCompletedHandle);
	TriggerOnFindSessionsCompleteDelegates(bSuccess);
}

bool FOnlineSessionPlayFab::IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId)
{
	return IsPlayerInSessionImpl(this, SessionName, UniqueId);
}

bool FOnlineSessionPlayFab::StartMatchmaking(const TArray< TSharedRef<const FUniqueNetId> >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::StartMatchmaking()"));
	
	if (LocalPlayers.Num() == 0)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::StartMatchmaking: LocalPlayers was empty. At least one player is required for matchmaking."));
		TriggerOnMatchmakingCompleteDelegates(SessionName, false);
		return false;
	}

	OnMatchmakingTicketCompletedDelegateHandle = FOnMatchmakingTicketCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnMatchmakingTicketCompleted);
	OnMatchmakingTicketCompletedHandle = OSSPlayFab->GetMatchmakingInterface()->AddOnMatchmakingTicketCompletedDelegate_Handle(OnMatchmakingTicketCompletedDelegateHandle);

	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::StartMatchmaking:CreateMatchMakingTicket()"));
	if (!OSSPlayFab->GetMatchmakingInterface()->CreateMatchMakingTicket(LocalPlayers, SessionName, NewSessionSettings, SearchSettings))
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::StartMatchmaking: Failed to CreateMatchMakingTicket"));
		TriggerOnMatchmakingCompleteDelegates(SessionName, false);
		return false;
	}

	return true;
}

void FOnlineSessionPlayFab::OnMatchmakingTicketCompleted(bool fSuccess, FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnMatchmakingTicketCompleted()"));
	OSSPlayFab->GetMatchmakingInterface()->ClearOnMatchmakingTicketCompletedDelegate_Handle(OnMatchmakingTicketCompletedHandle);
	OnMatchmakingComplete(SessionName, fSuccess);
}

void FOnlineSessionPlayFab::OnCancelMatchmakingComplete(FName SessionName, bool fSuccess)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCancelMatchmakingComplete()"));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, fSuccess);
	OSSPlayFab->GetMatchmakingInterface()->ClearOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteHandle);
}

void FOnlineSessionPlayFab::OnMatchmakingComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnMatchmakingComplete()"));
	MatchmakingCompleteSessionName = SessionName;

	if (OSSPlayFab == nullptr)
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnMatchmakingComplete: OSSPlayFab was null"));
		TriggerOnMatchmakingCompleteDelegates(SessionName, false);
		return;
	}

	if (bWasSuccessful == false)
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnMatchmakingComplete: Matchmaking was not successful"));
		TriggerOnMatchmakingCompleteDelegates(SessionName, false);
		return;
	}

	if (FNamedOnlineSession* Session = GetNamedSession(SessionName))
	{
		if (Session->bHosting)
		{
			OSSPlayFab->AddOnPartyEndpointCreatedDelegate_Handle(FOnPartyEndpointCreatedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking));
			bool bSuccess = OSSPlayFab->CreateAndConnectToPlayFabPartyNetwork();

			if (bSuccess == false)
			{
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnMatchmakingComplete: CreateAndConnectToPlayFabPartyNetwork failed"));
				TriggerOnMatchmakingCompleteDelegates(SessionName, false);
			}
		}
		else
		{
			//kick off the join logic and give time for the session update
			RetryJoinMatchmakingSession_Count = RetryJoinMatchmakingSession_MaxCount;
		}
		// Matchmaking does not use native interface
#if defined(USES_NATIVE_SESSION)
		bUsesNativeSession = false;
#endif
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnMatchmakingComplete: Session was null"));
		TriggerOnMatchmakingCompleteDelegates(SessionName, false);
	}
}

void FOnlineSessionPlayFab::OnOperationComplete_TryJoinNetwork(bool bJoinLobbyOperation, int32& RetryOperationSession_Count)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnOperationComplete_TryJoinNetwork()"));

	FName SessionName = bJoinLobbyOperation ? JoinSessionCompleteSessionName : MatchmakingCompleteSessionName;

	if (OSSPlayFab == nullptr)
	{
		//prevent any retry attempts
		RetryOperationSession_Count = 0;

		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnOperationComplete_TryJoinNetwork: OSSPlayFab was null"));
		if (bJoinLobbyOperation)
		{
			TriggerOnJoinSessionCompleteDelegates(JoinSessionCompleteSessionName, EOnJoinSessionCompleteResult::UnknownError);
		}
		else
		{
			TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
		}

		return;
	}

	if (FOnlineSessionSettings* SessionSettings = GetSessionSettings(SessionName))
	{
		FString NetworkIdStr;
		FString NetworkDescriptorStr;
		FString HostConnectInfo;
		if (SessionSettings->Get(SETTING_NETWORK_ID, NetworkIdStr) &&
			SessionSettings->Get(SETTING_NETWORK_DESCRIPTOR, NetworkDescriptorStr) &&
			SessionSettings->Get(SETTING_HOST_CONNECT_INFO, HostConnectInfo))
		{
			if (bJoinLobbyOperation)
			{
				OSSPlayFab->AddOnPartyEndpointCreatedDelegate_Handle(FOnPartyEndpointCreatedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCreatePartyEndpoint_JoinSession));
			}
			else
			{
				OSSPlayFab->AddOnPartyEndpointCreatedDelegate_Handle(FOnPartyEndpointCreatedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking));
			}

			if (!OSSPlayFab->ConnectToPlayFabPartyNetwork(NetworkIdStr, NetworkDescriptorStr))
			{
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnOperationComplete_TryJoinNetwork: ConnectToPlayFabPartyNetwork failed"));
				if (bJoinLobbyOperation)
				{
					TriggerOnJoinSessionCompleteDelegates(JoinSessionCompleteSessionName, EOnJoinSessionCompleteResult::UnknownError);
				}
				else
				{
					TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
				}
			}

			//prevent any retry attempts
			RetryOperationSession_Count = 0;
		}
		else
		{
			//stop retrying if we are out of attempts
			if (RetryOperationSession_Count == 0)
			{
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnOperationComplete_TryJoinNetwork: Missing required session settings"));
				if (bJoinLobbyOperation)
				{
					TriggerOnJoinSessionCompleteDelegates(JoinSessionCompleteSessionName, EOnJoinSessionCompleteResult::UnknownError);
				}
				else
				{
					TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
				}
			}
		}
	}
	else
	{
		//prevent any retry attempts
		RetryOperationSession_Count = 0;

		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnOperationComplete_TryJoinNetwork: SessionSettings was null"));
		if (bJoinLobbyOperation)
		{
			TriggerOnJoinSessionCompleteDelegates(JoinSessionCompleteSessionName, EOnJoinSessionCompleteResult::UnknownError);
		}
		else
		{
			TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
		}
	}
}

void FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking(bool bSuccess, uint16 EndpointID, bool bIsHosting)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking()"));

	OSSPlayFab->ClearOnPartyEndpointCreatedDelegates(this);

	if (bIsHosting)
	{
		if (FOnlineSessionSettings* SessionSettings = GetSessionSettings(MatchmakingCompleteSessionName))
		{
			if (ISocketSubsystem* PlayFabSocketSubsystem = ISocketSubsystem::Get(PLAYFAB_SOCKET_SUBSYSTEM))
			{
				bool BindAll = false;
				TSharedRef<FInternetAddr> LocalIp = PlayFabSocketSubsystem->GetLocalHostAddr(*GLog, BindAll);
				if (LocalIp->IsValid())
				{
					FString NetworkIdStr = OSSPlayFab->NetworkId;
					FString NetworkDescriptorStr = OSSPlayFab->SerializeNetworkDescriptor(OSSPlayFab->NetworkDescriptor);
					FString HostConnectInfo = LocalIp->ToString(false);

					SessionSettings->Set(SETTING_NETWORK_ID, NetworkIdStr, EOnlineDataAdvertisementType::ViaOnlineService);
					SessionSettings->Set(SETTING_NETWORK_DESCRIPTOR, NetworkDescriptorStr, EOnlineDataAdvertisementType::ViaOnlineService);
					SessionSettings->Set(SETTING_HOST_CONNECT_INFO, HostConnectInfo, EOnlineDataAdvertisementType::ViaOnlineService);

					OnUpdateSession_MatchmakingDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnUpdateLobbyCompletedDelegate_Handle(FOnUpdateLobbyCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnUpdateSession_Matchmaking));
					if (!OSSPlayFab->GetPlayFabLobbyInterface()->UpdateLobby(MatchmakingCompleteSessionName, *SessionSettings))
					{
						UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking: Failed to update Lobby with network descriptor %s"), *MatchmakingCompleteSessionName.ToString());
						OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnUpdateLobbyCompletedDelegate_Handle(OnUpdateSession_MatchmakingDelegateHandle);
						TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
					}
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking: LocalIp was invalid"));
					TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
				}
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking: PlayFabSocketSubsystem was null"));
				TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking: SessionSettings was null"));
			TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
		}
	}
	else
	{
		TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, bSuccess);
	}
}

void FOnlineSessionPlayFab::OnCreatePartyEndpoint_JoinSession(bool bSuccess, uint16 EndpointID, bool bIsHosting)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_JoinSession()"));
	OSSPlayFab->ClearOnPartyEndpointCreatedDelegates(this);
	TriggerOnJoinSessionCompleteDelegates(JoinSessionCompleteSessionName, bSuccess ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
}

void FOnlineSessionPlayFab::OnUpdateLobbyCompleted(FName SessionName, bool bWasSuccessful)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnUpdateLobbyCompleted()"));
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnUpdateLobbyCompletedDelegate_Handle(OnUpdateLobbyCompleteDelegate);
	TriggerOnUpdateSessionCompleteDelegates(SessionName, bWasSuccessful);
}

void FOnlineSessionPlayFab::OnUpdateSession_Matchmaking(FName SessionName, bool bWasSuccessful)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnUpdateSession_Matchmaking()"));
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnUpdateLobbyCompletedDelegate_Handle(OnUpdateSession_MatchmakingDelegateHandle);
	TriggerOnMatchmakingCompleteDelegates(SessionName, bWasSuccessful);
}

bool FOnlineSessionPlayFab::CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::CancelMatchmaking(index)"));

	auto allUsers = OSSPlayFab->GetIdentityInterface()->GetAllUserAccounts();
	if (allUsers.Num() > SearchingPlayerNum)
	{
		auto eachUser = allUsers[SearchingPlayerNum];
		return CancelMatchmaking(*eachUser->GetUserId(), SessionName);
	}

	UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::CancelMatchmaking: SearchingPlayerNum index invalid: %i"), SearchingPlayerNum);
	return false;
}

bool FOnlineSessionPlayFab::CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::CancelMatchmaking()"));

	OnCancelMatchmakingCompleteDelegateHandle = FOnCancelMatchmakingCompleteDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCancelMatchmakingComplete);
	OnCancelMatchmakingCompleteHandle = OSSPlayFab->GetMatchmakingInterface()->AddOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegateHandle);

	return OSSPlayFab->GetMatchmakingInterface()->CancelMatchmakingTicket(SessionName);
}

bool FOnlineSessionPlayFab::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::FindSessions()"));

	TSharedPtr<const FUniqueNetId> PlayerId;
	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		return false;
	}

	PlayerId = IdentityIntPtr->GetUniquePlayerId(SearchingPlayerNum);
	if (!PlayerId.IsValid())
	{
		return false;
	}

	return FindSessions(*PlayerId, SearchSettings);
}

bool FOnlineSessionPlayFab::FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::FindSessions()"));

	OnFindLobbiesCompletedDelegateHandle = FOnFindLobbiesCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnFindLobbiesCompleted);
	OnFindLobbiesCompletedHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnFindLobbiesCompletedDelegate_Handle(OnFindLobbiesCompletedDelegateHandle);

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->FindLobbies(SearchingPlayerId, SearchSettings))
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::FindSessions: Failed to FindLobbies"));
		OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnFindLobbiesCompletedDelegate_Handle(OnFindLobbiesCompletedHandle);
		TriggerOnFindSessionsCompleteDelegates(false);
		return false;
	}

#if defined(OSS_PLAYFAB_WIN64)
	CachedSearchSettings = MakeShared<FOnlineSessionSearch>(*SearchSettings);
	CachedSearchSettings->SearchState = EOnlineAsyncTaskState::NotStarted;
	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			OnNativeFindSessionsCompleteDelegateHandle = NativeSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
				FOnFindSessionsCompleteDelegate::CreateLambda([this, NativeSessionInterface](bool NativeSessionFindResult)
					{
						if (!NativeSessionFindResult)
						{
							UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeFindSessionComplete: Find Steam native sessions has failure"));
						}
						UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnNativeFindSessionComplete: Num of Search Results: %d"), CachedSearchSettings->SearchResults.Num());
						NativeSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnNativeFindSessionsCompleteDelegateHandle);
					}));
		return NativeSessionInterface->FindSessions(SearchingPlayerId, CachedSearchSettings.ToSharedRef());
		}
	}
#endif

	return true;
}

bool FOnlineSessionPlayFab::FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegate)
{
	// Unsupported
	CompletionDelegate.ExecuteIfBound(0, false, FOnlineSessionSearchResult());
	return false;
}

bool FOnlineSessionPlayFab::CancelFindSessions()
{
	// Unsupported
	OSSPlayFab->ExecuteNextTick([this]()
	{
		TriggerOnCancelFindSessionsCompleteDelegates(false);
	});

	return false;
}

bool FOnlineSessionPlayFab::PingSearchResults(const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG_ONLINE_SESSION(Display, TEXT("FOnlineSessionPlayFab::PingSearchResults: Not implemented"));
	return false;
}

void FOnlineSessionPlayFab::RegisterForUpdates()
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RegisterForUpdates()"));

	// Register for changes for this session
	OnLobbyUpdateDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyUpdateDelegate_Handle(FOnLobbyUpdateDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyUpdate));
	OnLobbyMemberAddedDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyMemberAddedDelegate_Handle(FOnLobbyMemberAddedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyMemberAdded));
	OnLobbyMemberRemovedDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyMemberRemovedDelegate_Handle(FOnLobbyMemberRemovedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyMemberRemoved));
	OnLobbyDisconnectedDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyDisconnectedDelegate_Handle(FOnLobbyDisconnectedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyDisconnected));
	
	if (bUsesNativeSession)
	{
		OnNativeSessionUserInviteAcceptedDelegate = FOnSessionUserInviteAcceptedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnNativeSessionUserInviteAccepted);
	}
}

void FOnlineSessionPlayFab::UnregisterForUpdates()
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UnregisterForUpdates()"));

	// clear session delegates
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyUpdateDelegate_Handle(OnLobbyUpdateDelegateHandle);
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyMemberAddedDelegate_Handle(OnLobbyMemberAddedDelegateHandle);
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyMemberRemovedDelegate_Handle(OnLobbyMemberRemovedDelegateHandle);
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyDisconnectedDelegate_Handle(OnLobbyDisconnectedDelegateHandle);
	
	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			NativeSessionInterface->ClearOnSessionUserInviteAcceptedDelegates(this);
		}
	}
}

void FOnlineSessionPlayFab::OnLobbyUpdate(FName SessionName, const PFLobbyUpdatedStateChange& StateChange)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate()"));

	HRESULT Hr;
	FNamedOnlineSessionPtr ExistingNamedSession = GetNamedSessionPtr(SessionName);
	if (!ExistingNamedSession.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate No session found with SessionName:%s!"), *(SessionName.ToString()));
		return;
	}

	bool UpdateHostSetting = false;
	for (uint32 i = 0; i < StateChange.updatedLobbyPropertyCount; ++i)
	{
		const char* updatedLobbyKey = StateChange.updatedLobbyPropertyKeys[i];
		const char* updatedLobbyPropertyValue = nullptr;
		Hr = PFLobbyGetLobbyProperty(StateChange.lobby, updatedLobbyKey, &updatedLobbyPropertyValue);
		if (FAILED(Hr))
		{
			LogMultiplayerErrorWithMessage("PFLobbyGetLobbyProperty", Hr);
			continue;
		}

		const FString UpdatedLobbyKey(UTF8_TO_TCHAR(updatedLobbyKey));

		if (updatedLobbyPropertyValue == nullptr)
		{
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Remove Key:%s"), *UpdatedLobbyKey);
			ExistingNamedSession->SessionSettings.Remove(FName(UpdatedLobbyKey));
		}
		else
		{
			const FString UpdatedLobbyPropertyValue(UTF8_TO_TCHAR(updatedLobbyPropertyValue));

			if (UpdatedLobbyKey == TEXT("_flags"))
			{
				int16 SessionSettingsFlags = 0;
				LexFromString(SessionSettingsFlags, *UpdatedLobbyPropertyValue);

				int32 BitShift = 0;
				ExistingNamedSession->SessionSettings.bShouldAdvertise = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bAllowJoinInProgress = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bIsLANMatch = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bIsDedicated = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bUsesStats = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bAllowInvites = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bUsesPresence = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bAllowJoinViaPresence = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bAllowJoinViaPresenceFriendsOnly = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
				ExistingNamedSession->SessionSettings.bAntiCheatProtected = (SessionSettingsFlags & (1 << BitShift++)) ? true : false;
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Lobby Key:%s, value:%s"), *UpdatedLobbyKey, *UpdatedLobbyPropertyValue);
				ExistingNamedSession->SessionSettings.Set(FName(UpdatedLobbyKey), UpdatedLobbyPropertyValue, EOnlineDataAdvertisementType::ViaOnlineService);
				if (IsHostSetting(FName(UpdatedLobbyKey)))
				{
					UpdateHostSetting = true;
				}
			}
		}
	}

	for (uint32 i = 0; i < StateChange.updatedSearchPropertyCount; ++i)
	{
		const char* updatedSearchKey = StateChange.updatedSearchPropertyKeys[i];
		const char* updatedSearchPropertyValue = nullptr;
		Hr = PFLobbyGetSearchProperty(StateChange.lobby, updatedSearchKey, &updatedSearchPropertyValue);
		if (FAILED(Hr))
		{
			LogMultiplayerErrorWithMessage("PFLobbyGetSearchProperty", Hr);
			continue;
		}

		const FString UpdatedSearchKey(UTF8_TO_TCHAR(updatedSearchKey));

		if (updatedSearchPropertyValue == nullptr)
		{
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Remove Key:%s"), *UpdatedSearchKey);
			ExistingNamedSession->SessionSettings.Remove(FName(UpdatedSearchKey));
		}
		else
		{
			const FString UpdatedSearchPropertyValue(UTF8_TO_TCHAR(updatedSearchPropertyValue));

			// return search properties back to session settings
			auto SettingKey = OSSPlayFab->GetPlayFabLobbyInterface()->FindSearchKey(UpdatedSearchKey);
			if (SettingKey)
			{
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Search Key:%s, value:%s"), *FString(SettingKey->Key), *UpdatedSearchPropertyValue);
				switch (SettingKey->Value)
				{
					case EOnlineKeyValuePairDataType::Bool:
						ExistingNamedSession->SessionSettings.Set(FName(SettingKey->Key), *updatedSearchPropertyValue == '1' ? true : false, EOnlineDataAdvertisementType::ViaOnlineService);
						break;
					case EOnlineKeyValuePairDataType::Int32:
						ExistingNamedSession->SessionSettings.Set(FName(SettingKey->Key), FCString::Atoi(*UpdatedSearchPropertyValue), EOnlineDataAdvertisementType::ViaOnlineService);
						break;
					case EOnlineKeyValuePairDataType::String:
						ExistingNamedSession->SessionSettings.Set(FName(SettingKey->Key), UpdatedSearchPropertyValue, EOnlineDataAdvertisementType::ViaOnlineService);
						break;
				}
				if (IsHostSetting(FName(SettingKey->Key)))
				{
					UpdateHostSetting = true;
				}
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Search Key:%s, value:%s"), *UpdatedSearchKey, *UpdatedSearchPropertyValue);
				ExistingNamedSession->SessionSettings.Set(FName(UpdatedSearchKey), UpdatedSearchPropertyValue, EOnlineDataAdvertisementType::ViaOnlineService);
			}
		}
	}

	if (StateChange.ownerUpdated || UpdateHostSetting)
	{
		const PFEntityKey* OwnerEntityKeyPtr;
		Hr = PFLobbyGetOwner(StateChange.lobby, &OwnerEntityKeyPtr);
		if (SUCCEEDED(Hr))
		{
			if (OwnerEntityKeyPtr != nullptr)
			{
				if (!SetHostOnSession(SessionName, *OwnerEntityKeyPtr))
				{
					UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate invalid owner!"));
				}
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate owner is removed from the session!"));
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate failed to GetOwner for the lobby. ErrorCode=[0x%08x], Warning message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		}
	}

	if (StateChange.maxMembersUpdated)
	{
		uint32 NewMemberCount;
		Hr = PFLobbyGetMaxMemberCount(StateChange.lobby, &NewMemberCount);
		if (SUCCEEDED(Hr))
		{
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate max member count updated to :%u"), NewMemberCount);
			ExistingNamedSession->SessionSettings.NumPublicConnections = NewMemberCount;
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate failed to GetMaxMemberCount for the lobby. ErrorCode=[0x%08x], Warning message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		}
	}

	if (StateChange.accessPolicyUpdated)
	{
		PFLobbyAccessPolicy NewAccessPolicy;
		Hr = PFLobbyGetAccessPolicy(StateChange.lobby, &NewAccessPolicy);
		if (SUCCEEDED(Hr))
		{
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate access policy updated to :%u"), NewAccessPolicy);
			ExistingNamedSession->SessionSettings.bShouldAdvertise = (NewAccessPolicy != PFLobbyAccessPolicy::Private);
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate failed to GetAccessPolicy for the lobby. ErrorCode=[0x%08x], Warning message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		}
	}

	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate member update count:%u"), StateChange.memberUpdateCount);
	for (uint32_t i = 0; i < StateChange.memberUpdateCount; ++i)
	{
		const PFLobbyMemberUpdateSummary& MemberUpdate = StateChange.memberUpdates[i];
		const PFEntityKey* MemberEntity = &MemberUpdate.member;		
		const FString* PlatformId = EntityPlatformIdMapping.Find(FString(MemberEntity->id));

		if ((PlatformId != nullptr) && (!PlatformId->IsEmpty()))
		{
			FSessionSettings* MemberSettings = ExistingNamedSession->SessionSettings.MemberSettings.Find(FUniqueNetIdPlayFab::Create(*PlatformId));
			if (!MemberSettings)
			{
				MemberSettings = &ExistingNamedSession->SessionSettings.MemberSettings.Add(FUniqueNetIdPlayFab::Create(*PlatformId), FSessionSettings());
			}

			for (uint32_t j = 0; j < MemberUpdate.updatedMemberPropertyCount; ++j)
			{
				const char* Key = MemberUpdate.updatedMemberPropertyKeys[j];
				const char* Value = nullptr;
				Hr = PFLobbyGetMemberProperty(StateChange.lobby, MemberEntity, Key, &Value);
				if (SUCCEEDED(Hr))
				{
					MemberSettings->Add(FName(Key), FOnlineSessionSetting(Value != nullptr ? FString(UTF8_TO_TCHAR(Value)) : FString(), EOnlineDataAdvertisementType::ViaOnlineService));
					UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Update member property for:%s, Key:%s Value:%s"), *FString(MemberEntity->id), *FString(UTF8_TO_TCHAR(Key)), *FString(UTF8_TO_TCHAR(Value)));
				}
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate PlatformId not found for Entity:%s!"), *FString(MemberEntity->id));
		}
	}

#if defined(OSS_PLAYFAB_GDK)
	SetMultiplayerActivityForSession(ExistingNamedSession);
#endif

	TriggerOnSessionSettingsUpdatedDelegates(SessionName, ExistingNamedSession->SessionSettings);
}

void FOnlineSessionPlayFab::OnLobbyMemberAdded(FName SessionName, const PFLobbyMemberAddedStateChange& StateChange)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyMemberAdded()"));

	FNamedOnlineSessionPtr ExistingNamedSession = GetNamedSessionPtr(SessionName);
	if (!ExistingNamedSession.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyMemberAdded: No session found with SessionName:%s!"), *(SessionName.ToString()));
		return;
	}

	const PFEntityKey& EntityKey = StateChange.member;
	
	const char* PlatformIdValue = nullptr;
	HRESULT Hr = PFLobbyGetMemberProperty(StateChange.lobby, &EntityKey, TCHAR_TO_UTF8(*SETTING_PLATFORM_ID), &PlatformIdValue);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFLobbyGetMemberProperty", Hr);
		return;
	}

	if (PlatformIdValue == nullptr)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyMemberAdded: PlatformIdValue was null"));
		return;
	}

	const char* PlatformModelValue = nullptr;
	Hr = PFLobbyGetMemberProperty(StateChange.lobby, &EntityKey, TCHAR_TO_UTF8(*SETTING_PLATFORM_MODEL), &PlatformModelValue);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFLobbyGetMemberProperty", Hr);
		return;
	}

	if (PlatformModelValue == nullptr)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyMemberAdded: PlatformModelValue was null"));
		return;
	}
	
	const FString EntityIdStr(UTF8_TO_TCHAR(EntityKey.id));
	const FString PlatformIdStr(UTF8_TO_TCHAR(PlatformIdValue));
	const FString PlatformModelStr(UTF8_TO_TCHAR(PlatformModelValue));

	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyMemberAdded Id:%s, PlatformId:%s, PlatformModel:%s"), *EntityIdStr, *PlatformIdStr, *PlatformModelStr);
	EntityPlatformIdMapping.Add(EntityIdStr, PlatformIdStr);

	auto VoicePlayFab = StaticCastSharedPtr<FOnlineVoicePlayFab>(OSSPlayFab->GetVoiceInterface());
	if (VoicePlayFab.IsValid())
	{
		VoicePlayFab->AddTalkerIdMapping(EntityIdStr, PlatformIdStr);

		if (ECrossNetworkType* ChatType = VoiceChatPlatforms.Find(PlatformModelStr))
		{
			VoicePlayFab->SetTalkerCrossNetworkPermission(*ChatType, PlatformIdStr, PlatformModelStr);
		}
	}

#if defined(OSS_PLAYFAB_GDK)
	FOnlineIdentityPlayFabPtr PlayFabIdentityInt = OSSPlayFab ? OSSPlayFab->GetIdentityInterfacePlayFab() : nullptr;
	if (PlayFabIdentityInt.IsValid())
	{
		RecordRecentlyMetPlayer(StateChange.lobby, PlayFabIdentityInt->GetLocalUserEntityKeys(), EntityKey, PlatformIdStr);
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyMemberAdded: Identity Interface is invalid"));
	}

	SetMultiplayerActivityForSession(ExistingNamedSession);
#endif

	const TSharedRef<const FUniqueNetId> MemberAddedNetId = FUniqueNetIdPlayFab::Create(PlatformIdStr);
	TriggerOnSessionParticipantJoinedDelegates(SessionName, *MemberAddedNetId);
}

void FOnlineSessionPlayFab::OnLobbyMemberRemoved(FName SessionName, const PFLobbyMemberRemovedStateChange& StateChange)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyMemberRemoved()"));

	FNamedOnlineSessionPtr ExistingNamedSession = GetNamedSessionPtr(SessionName);
	if (!ExistingNamedSession.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyMemberRemoved: No session found with SessionName:%s!"), *(SessionName.ToString()));
		return;
	}
	
	const PFEntityKey& MemberRemovedEntity = StateChange.member;
	const FString MemberRemovedEntityIdStr(UTF8_TO_TCHAR(MemberRemovedEntity.id));
	const FString MemberRemovedEntityTypeStr(UTF8_TO_TCHAR(MemberRemovedEntity.type));
	const FString* PlatformId = EntityPlatformIdMapping.Find(MemberRemovedEntityIdStr);
	if (PlatformId != nullptr && !PlatformId->IsEmpty())
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyMemberRemoved Id:%s, Type:%s PlatformId:%s"), *MemberRemovedEntityIdStr, *MemberRemovedEntityTypeStr, **PlatformId);
		const TSharedRef<const FUniqueNetId> MemberRemovedNetId = FUniqueNetIdPlayFab::Create(**PlatformId);
		EntityPlatformIdMapping.Remove(MemberRemovedEntityIdStr);
		TriggerOnSessionParticipantLeftDelegates(SessionName, *MemberRemovedNetId, EOnSessionParticipantLeftReason::Left);
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLobbyMemberRemoved failed to get PlatformId for Entity:%s!"), *MemberRemovedEntityIdStr);
	}

#if defined(OSS_PLAYFAB_GDK)
	SetMultiplayerActivityForSession(ExistingNamedSession);
#endif
}

void FOnlineSessionPlayFab::OnLobbyDisconnected(FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyDisconnected SessionName:%s"), *(SessionName.ToString()));

	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	// if session is still valid then disconnect was due to connection failure
	if (Session.IsValid())
	{
		// Leave the PlayFab Party network
		OSSPlayFab->LeavePlayFabPartyNetwork();

		RemoveNamedSession(SessionName);
		TriggerOnSessionFailureDelegates(*FUniqueNetIdPlayFab::EmptyId(), ESessionFailure::ServiceConnectionLost);
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyDisconnected SessionName:%s disconnected due to ServiceConnectionLost"), *(SessionName.ToString()));
	}

	// NOTE: This implmentation is based on the assumption that we only have one lobby.
	if (OnSessionsRemovedDelegate.IsBound())
	{
		OnSessionsRemovedDelegate.Execute();
		OnSessionsRemovedDelegate.Unbind();
	}
}

bool FOnlineSessionPlayFab::SetHostOnSession(FName SessionName, const PFEntityKey& HostEntityKey)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SetHostOnSession()"));

	FNamedOnlineSessionPtr ExistingNamedSession = GetNamedSessionPtr(SessionName);
	if (!ExistingNamedSession.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetHostOnSession No session found with SessionName: %s!"), *(SessionName.ToString()));
		return false;
	}

	if (FCString::Strcmp(UTF8_TO_TCHAR(HostEntityKey.type), UTF8_TO_TCHAR(ENTITY_TYPE_TITLE_PLAYER)) != 0)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetHostOnSession owner type is not title_player_account: %s!"), UTF8_TO_TCHAR(HostEntityKey.type));
	}

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetHostOnSession Identity Interface is invalid"));
		return false;
	}	

	FString* HostPlatformId = EntityPlatformIdMapping.Find(FString(HostEntityKey.id));
	if (HostPlatformId == nullptr)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetHostOnSession couldn't find a new lobby owner in the entity mapping cache!"));
		return false;
	}

	FString PlayerNickname;
	ExistingNamedSession->SessionSettings.Get(SETTING_HOST_NICKNAME, PlayerNickname);
	if (PlayerNickname.IsEmpty())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetHostOnSession: session owner nickname is not found in session %s for player with ID %s"), *SessionName.ToString(), *(*HostPlatformId));
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetHostOnSession: session owner nickname is %s"), *PlayerNickname);
	}

	ExistingNamedSession->OwningUserId = FUniqueNetIdPlayFab::Create(*HostPlatformId);
	ExistingNamedSession->OwningUserName = PlayerNickname;

	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SetHostOnSession set host updated:%s"), *ExistingNamedSession->OwningUserId->ToString());

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformIdString(*HostPlatformId);

	if (LocalUser == nullptr)
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SetHostOnSession Identity Interface's GetPartyLocalUserFromPlatformId returned empty user. Local user is not the host."));
	}
	else
	{
		ExistingNamedSession->bHosting = true;
		ExistingNamedSession->LocalOwnerId = ExistingNamedSession->OwningUserId->AsShared();
	}

	return true;
}

bool FOnlineSessionPlayFab::JoinSession_PlayFabInternal(int32 ControllerIndex, TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::JoinSession_PlayFabInternal()"));

	OnJoinLobbyCompleteDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnJoinLobbyCompletedDelegate_Handle(FOnJoinLobbyCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnJoinLobbyCompleted));

	TSharedPtr<const FUniqueNetId> PlayerId;
	if (ControllerIndex != INDEX_NONE)
	{
		IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
		if (IdentityIntPtr.IsValid())
		{
			PlayerId = IdentityIntPtr->GetUniquePlayerId(ControllerIndex);
			if (!PlayerId.IsValid())
			{
				TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::UnknownError);
				return false;
			}
		}
	}
	else if (UserId.IsValid())
	{
		PlayerId = UserId;
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession_PlayFabInternal: invalid data to join the session"));
		TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		return false;
	}

	// work out if we're already in the session of this name or not
	FNamedOnlineSessionPtr NamedSessionCheck = GetNamedSessionPtr(SessionName);
	if (NamedSessionCheck.IsValid())
	{
		// Check if we're trying to join a different session of this same type while in a different session (different than joining the same session multiple times)
		const FString ExistingSessionId(NamedSessionCheck->GetSessionIdStr());
		const FString NewSessionId(DesiredSession.GetSessionIdStr());

		if (ExistingSessionId == NewSessionId)
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession_PlayFabInternal: Join session failed; session (%s) already exists, can't join twice"), *SessionName.ToString());
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession_PlayFabInternal: Join session failed; already in session of type %s, you must leave session %s before joining %s"), *SessionName.ToString(), *ExistingSessionId, *NewSessionId);
		}

		TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::AlreadyInSession);
		return false;
	}

	FNamedOnlineSessionRef NamedSession = AddNamedSessionRef(SessionName, DesiredSession.Session);
	NamedSession->HostingPlayerNum = INDEX_NONE;
	NamedSession->OwningUserId = PlayerId;
	NamedSession->LocalOwnerId = PlayerId;

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->JoinLobby(*PlayerId, SessionName, DesiredSession))
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::JoinSession_PlayFabInternal: Failed to JoinLobby"));
		RemoveNamedSession(SessionName);
		TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		return false;
	}

	return true;
}

bool FOnlineSessionPlayFab::JoinSession(int32 ControllerIndex, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::JoinSession()"));

	bool bSuccess = JoinSession_PlayFabInternal(ControllerIndex, nullptr, SessionName, DesiredSession);
	if (bSuccess == false)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: Failed to join session"));
		TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		return false;
	}

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			OnNativeJoinSessionCompleteDelegateHandle = NativeSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
				FOnJoinSessionCompleteDelegate::CreateLambda([this, NativeSessionInterface](FName SessionName, EOnJoinSessionCompleteResult::Type NativeSessionJoinedResult)
					{
						if (NativeSessionJoinedResult != EOnJoinSessionCompleteResult::Success)
						{
							UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeJoinSessionComplete: Failed to join native session due to %s"), LexToString(NativeSessionJoinedResult));
						}

						NativeSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnNativeJoinSessionCompleteDelegateHandle);
					}));
			return NativeSessionInterface->JoinSession(ControllerIndex, NativeSessionName, DesiredSession);
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: Failed to get native session (%s) interface"), *NativeSessionName.ToString());
			return false;
		}
	}

	return true;
}

bool FOnlineSessionPlayFab::JoinSession(const FUniqueNetId& UserId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::JoinSession()"));

	bool bSuccess = JoinSession_PlayFabInternal(INDEX_NONE, UserId.AsShared(), SessionName, DesiredSession);
	if (bSuccess == false)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: Failed to join session"));
		TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::UnknownError);
		return false;
	}

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			// Determine if DesiredSession.Session.SessionInfo is type of FOnlineSessionInfoPlayfab.
			// If yes, meaning this call triggered after FindSessions(), othewise, this call is triggered
			// by invitation from native layer.
			if (DesiredSession.Session.SessionInfo->GetSessionId().GetType() == PLAYFAB_SUBSYSTEM)
			{
				FOnlineSessionInfoPlayFabPtr PlayFabSessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(DesiredSession.Session.SessionInfo);
				if (PlayFabSessionInfo.IsValid())
				{
#if defined(OSS_PLAYFAB_PLAYSTATION)
					if (PlayFabSessionInfo->GetNativePlatform() != PLATFORM_MODEL_SONY)
					{
						// The host is a not a PlayStation player and not joining host's native session is expected.
						UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::JoinSession: Won't join a native session which is hosted by the player from different platform."));
						bUsesNativeSession = false;
						return true;
					}
					const FString SessionIdString(PlayFabSessionInfo->GetNativeSessionIdString());
					if (!SessionIdString.IsEmpty())
					{
						if (FUniqueNetIdPtr SessionId = NativeSessionInterface->CreateSessionIdFromString(SessionIdString))
						{
							bSuccess = NativeSessionInterface->FindSessionById(UserId, *SessionId, *CreateNativeNetIdPtr(), TEXT(""), FOnSingleSessionResultCompleteDelegate::CreateLambda([this, &UserId, SessionName, NativeSessionInterface, &bSuccess](int32, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
								{
									if (bWasSuccessful)
									{
										bSuccess = NativeSessionInterface->JoinSession(UserId, NativeSessionName, SearchResult);
										if (!bSuccess)
										{
											UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: Found Playstation native session but failed to join it"));
										}
									}
									else
									{
										UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: Failed to find native session"));
										bSuccess = false;
									}
								}));
						}
						else
						{
							UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: Failed to create Playstation session from string %s"), SessionIdString.IsEmpty() ? TEXT("nullptr") : *SessionIdString);
							bSuccess = false;
						}
					}
					else
					{
						UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: SessionIdString is empty"));
						bSuccess = false;
					}
#elif defined(OSS_PLAYFAB_WIN64)
					if (PlayFabSessionInfo->GetNativePlatform() != PLATFORM_MODEL_WIN64)
					{
						// The host is a not a Steam player and not joining host's native session is expected.
						UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::JoinSession: Won't join a native session which is hosted by the player from different platform."));
						bUsesNativeSession = false;
						return true;
					}
					const FString SessionIdString(PlayFabSessionInfo->GetNativeSessionIdString());
					if (!SessionIdString.IsEmpty())
					{
						const FOnlineSessionSearchResult* NativeDesiredSession = nullptr;
						for (auto SearchResult : CachedSearchSettings->SearchResults)
						{
							if (SearchResult.Session.GetSessionIdStr() == SessionIdString)
							{
								NativeDesiredSession = &SearchResult;
								break;
							}
						}
						if (!NativeDesiredSession)
						{
							UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::JoinSession: Failed to find Steam session with sessionId=%s"), *SessionIdString);
							bSuccess = false;
						}
						else
						{
							OnNativeJoinSessionCompleteDelegateHandle = NativeSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
								FOnJoinSessionCompleteDelegate::CreateLambda([this, NativeSessionInterface](FName SessionName, EOnJoinSessionCompleteResult::Type NativeSessionJoinedResult)
									{
										if (NativeSessionJoinedResult != EOnJoinSessionCompleteResult::Success)
										{
											UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeJoinSessionComplete: Failed to join native session due to %s"), LexToString(NativeSessionJoinedResult));
										}
										NativeSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnNativeJoinSessionCompleteDelegateHandle);
									}));
							bSuccess = NativeSessionInterface->JoinSession(UserId, NativeSessionName, *NativeDesiredSession);
							}
						}
					else
					{
						UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: SessionIdString is empty"));
						bSuccess = false;
					}
#endif // !OSS_PLAYFAB_PLAYSTATION
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: PlayFabSessionInfo was invalid"));
					bSuccess = false;
				}
				return bSuccess;
			}
			else
			{
				// This call is triggered by invitation from native layer.
				OnNativeJoinSessionCompleteDelegateHandle = NativeSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
					FOnJoinSessionCompleteDelegate::CreateLambda([this, NativeSessionInterface](FName SessionName, EOnJoinSessionCompleteResult::Type NativeSessionJoinedResult)
						{
							if (NativeSessionJoinedResult != EOnJoinSessionCompleteResult::Success)
							{
								UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeJoinSessionComplete: Failed to join native session due to %s"), LexToString(NativeSessionJoinedResult));
							}

							NativeSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnNativeJoinSessionCompleteDelegateHandle);
						}));
				return NativeSessionInterface->JoinSession(UserId, NativeSessionName, DesiredSession);
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::JoinSession: Failed to get native session (%s) interface"), *NativeSessionName.ToString());
			return false;
		}

	}

	return true;
}

void FOnlineSessionPlayFab::OnJoinLobbyCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnJoinLobbyCompleted()"));

	if (OSSPlayFab)
	{
		OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnJoinLobbyCompletedDelegate_Handle(OnJoinLobbyCompleteDelegateHandle);
	}

	JoinSessionCompleteSessionName = InSessionName;

	//kick off the join logic and give time for the session update
	RetryJoinLobbySession_Count = RetryJoinLobbySession_MaxCount;

	if (Result != EOnJoinSessionCompleteResult::Success && Result != EOnJoinSessionCompleteResult::AlreadyInSession)
	{
		// Leave the PlayFab Party network
		OSSPlayFab->LeavePlayFabPartyNetwork();
		
		TriggerOnJoinSessionCompleteDelegates(InSessionName, Result);
	}
}

bool FOnlineSessionPlayFab::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::FindFriendSession()"));

	TSharedPtr<const FUniqueNetId> PlayerId;
	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		return false;
	}

	PlayerId = IdentityIntPtr->GetUniquePlayerId(LocalUserNum);
	if (!PlayerId.IsValid())
	{
		return false;
	}

	return FindFriendSession(*PlayerId, Friend);
}

bool FOnlineSessionPlayFab::FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::FindFriendSession()"));

	OnFindFriendLobbiesCompletedHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnFindLobbiesCompletedDelegate_Handle(FOnFindLobbiesCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnFindFriendLobbiesCompleted));

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->FindFriendLobbies(LocalUserId))
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::FindFriendSession: Failed!"));
		OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnFindLobbiesCompletedDelegate_Handle(OnFindFriendLobbiesCompletedHandle);
		TriggerOnFindFriendSessionCompleteDelegates(-1, false, TArray<FOnlineSessionSearchResult>());
		return false;
	}

	return true;
}

bool FOnlineSessionPlayFab::FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<TSharedRef<const FUniqueNetId>>& FriendList)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::FindFriendSession()"));

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		return false;
	}
	int32 LocalUserNum = IdentityIntPtr->GetPlatformUserIdFromUniqueNetId(LocalUserId);
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, false, TArray<FOnlineSessionSearchResult>());

	return false;
}

void FOnlineSessionPlayFab::OnFindFriendLobbiesCompleted(int32 LocalUserNum, bool bSuccess, TSharedPtr<FOnlineSessionSearch> SearchResults)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnFindFriendLobbiesCompleted()"));

	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnFindLobbiesCompletedDelegate_Handle(OnFindFriendLobbiesCompletedHandle);
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, bSuccess, SearchResults->SearchResults);
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SendSessionInviteToFriend()"));

	FOnlineIdentityPlayFabPtr PlayFabIdentityInt = OSSPlayFab ? OSSPlayFab->GetIdentityInterfacePlayFab() : nullptr;
	if (PlayFabIdentityInt.IsValid())
	{
		TSharedPtr<const FUniqueNetId> PlayerId = PlayFabIdentityInt->GetUniquePlayerId(LocalUserNum);
		if (PlayerId.IsValid())
		{
			return SendSessionInviteToFriend(*PlayerId, SessionName, Friend);
		}
	}

	return false;
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SendSessionInviteToFriend()"));

	if (Friend.IsValid())
	{
		TArray< TSharedRef<const FUniqueNetId> > Friends;
		Friends.Add(Friend.AsShared());
		return SendInvite(LocalUserId, SessionName, Friends);
	}

	return false;
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SendSessionInviteToFriends()"));

	FOnlineIdentityPlayFabPtr PlayFabIdentityInt = OSSPlayFab ? OSSPlayFab->GetIdentityInterfacePlayFab() : nullptr;
	if (PlayFabIdentityInt.IsValid())
	{
		TSharedPtr<const FUniqueNetId> PlayerId = PlayFabIdentityInt->GetUniquePlayerId(LocalUserNum);
		if (PlayerId.IsValid())
		{
			return SendInvite(*PlayerId, SessionName, Friends);
		}
	}

	return false;
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SendSessionInviteToFriends()"));

	return SendInvite(LocalUserId, SessionName, Friends);
}

bool FOnlineSessionPlayFab::GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType)
{
	ConnectInfo = "0.0.0.0:5000";
	return true;
}

bool FOnlineSessionPlayFab::GetResolvedConnectString(const class FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo)
{
	// NOTE: We don't care about the IP addr since PlayFab Party does not use them. Return empty IP instead of an actual IP or SDA. Don't call the base OSS.
	ConnectInfo = "0.0.0.0:5000";
	return true;
}

FOnlineSessionSettings* FOnlineSessionPlayFab::GetSessionSettings(FName SessionName)
{
	FNamedOnlineSessionPtr MySession = GetNamedSessionPtr(SessionName);
	return MySession.IsValid() ? &MySession->SessionSettings : nullptr;
}

bool FOnlineSessionPlayFab::RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RegisterPlayer()"));

	TArray<TSharedRef<const FUniqueNetId>> Players;
	Players.Add(FUniqueNetIdPlayFab::Create(PlayerId));
	return RegisterPlayers(SessionName, Players, bWasInvited);	
}

bool FOnlineSessionPlayFab::RegisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players, bool bWasInvited /*= false*/)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RegisterPlayers()"));

	bool bSuccess = false;
	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	if (Session.IsValid())
	{
		if (Session->SessionInfo.IsValid())
		{
			for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
			{
				const TSharedRef<const FUniqueNetId>& PlayerId = Players[PlayerIdx];

				FUniqueNetIdMatcher PlayerMatch(*PlayerId);
				if (Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch) == INDEX_NONE)
				{
					Session->RegisteredPlayers.Add(PlayerId);
					if (OSSPlayFab->IsLocalPlayer(*PlayerId))
					{
						FSessionSettings* MemberSettings = Session->SessionSettings.MemberSettings.Find(PlayerId);
						if (MemberSettings)
						{
							if (!MemberSettings->Find(FName("Registered")))
							{
								MemberSettings->Add(FName("Registered"), FOnlineSessionSetting(true, EOnlineDataAdvertisementType::ViaOnlineService));
								UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RegisterPlayers: Marking Player %s as registered in session %s"), *Players[PlayerIdx]->ToDebugString(), *SessionName.ToString());
							}
						}
					}
					RegisterVoice(*PlayerId);
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RegisterPlayers: Player %s already registered in session %s"), *Players[PlayerIdx]->ToDebugString(), *SessionName.ToString());
				}
			}
			bSuccess = true;
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::RegisterPlayers: No session info to join for session (%s)"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::RegisterPlayers: No game present to join for session (%s)"), *SessionName.ToString());
	}

	TriggerOnRegisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

void FOnlineSessionPlayFab::RegisterVoice(const FUniqueNetId& PlayerId)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RegisterVoice()"));

	// Register the player as a local talker with our own voice interface
	IOnlineVoicePtr VoiceIntPtr = OSSPlayFab->GetVoiceInterface();
	if (VoiceIntPtr.IsValid())
	{
		FOnlineVoicePlayFab* PlayFabVoiceInt = static_cast<FOnlineVoicePlayFab*>(VoiceIntPtr.Get());

		IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
		if (IdentityIntPtr.IsValid())
		{
			FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());

			TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(PlayerId);
			if (LocalUser != nullptr)
			{
				PlayFabVoiceInt->RegisterLocalTalker(PlayerId);
			}
			else
			{
				PlayFabVoiceInt->RegisterRemoteTalker(PlayerId);
			}
		}
	}
}

bool FOnlineSessionPlayFab::UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UnregisterPlayer()"));

	TArray<TSharedRef<const FUniqueNetId>> Players;
	Players.Add(FUniqueNetIdPlayFab::Create(PlayerId));
	return UnregisterPlayers(SessionName, Players);
}

bool FOnlineSessionPlayFab::UnregisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UnregisterPlayers()"));

	bool bSuccess = false;

	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	if (Session.IsValid())
	{
		if (Session->SessionInfo.IsValid())
		{
			for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
			{
				const TSharedRef<const FUniqueNetId>& PlayerId = Players[PlayerIdx];

				FUniqueNetIdMatcher PlayerMatch(*PlayerId);
				int32 RegistrantIndex = Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch);
				if (RegistrantIndex != INDEX_NONE)
				{
					Session->RegisteredPlayers.RemoveAtSwap(RegistrantIndex);
					UnregisterVoice(*PlayerId);
					// The unetdriver cleanup triggers this unregister before the OnSessionChanged event so trigger this delegate here.
					// the test in OnSessionChanged will remain as different underlying net systems will rely on it.
					TriggerOnSessionParticipantLeftDelegates(SessionName, *PlayerId, EOnSessionParticipantLeftReason::Left);
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::UnregisterPlayers: Player %s is not part of session (%s)"), *PlayerId->ToDebugString(), *SessionName.ToString());
				}
			}

			bSuccess = true;
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::UnregisterPlayers: No session info to leave for session (%s)"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::UnregisterPlayers: No game present to leave for session (%s)"), *SessionName.ToString());
	}

	TriggerOnUnregisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

void FOnlineSessionPlayFab::UnregisterVoice(const FUniqueNetId& PlayerId)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UnregisterVoice()"));

	IOnlineVoicePtr VoiceIntPtr = OSSPlayFab->GetVoiceInterface();
	if (VoiceIntPtr.IsValid())
	{
		FOnlineVoicePlayFab* PlayFabVoiceInt = static_cast<FOnlineVoicePlayFab*>(VoiceIntPtr.Get());

		IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
		if (IdentityIntPtr.IsValid())
		{
			FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
			TSharedPtr<FPlayFabUser> pLocalPartyUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(PlayerId);
			if (pLocalPartyUser != nullptr)
			{
				PlayFabVoiceInt->UnregisterLocalTalker(PlayerId);
			}
			else
			{
				PlayFabVoiceInt->UnregisterRemoteTalker(PlayerId);
			}
		}
	}
}

void FOnlineSessionPlayFab::RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RegisterLocalPlayer()"));

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->AddLocalPlayer(PlayerId, SessionName, Delegate))
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::RegisterLocalPlayer: Failed to add player to session"));
		Delegate.ExecuteIfBound(PlayerId, EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			NativeSessionInterface->RegisterLocalPlayer(PlayerId, NativeSessionName, Delegate);
		}
	}
}

void FOnlineSessionPlayFab::UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UnregisterLocalPlayer()"));

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->LeaveLobby(PlayerId, SessionName, FOnDestroySessionCompleteDelegate(), Delegate, false))
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::UnregisterLocalPlayer: Failed to unregister player from the session"));
		Delegate.ExecuteIfBound(PlayerId, false);
		return;
	}

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			NativeSessionInterface->UnregisterLocalPlayer(PlayerId, NativeSessionName, Delegate);
		}
	}
}

int32 FOnlineSessionPlayFab::GetNumSessions()
{
	return Sessions.Num();
}

void FOnlineSessionPlayFab::DumpSessionState()
{
	FScopeLock ScopeLock(&SessionLock);

	for (const FNamedOnlineSessionRef& Session : Sessions)
	{
		DumpNamedSession(&Session.Get());
	}
}

FNamedOnlineSession* FOnlineSessionPlayFab::AddNamedSession(FName SessionName, const FOnlineSessionSettings& SessionSettings)
{
	FScopeLock ScopeLock(&SessionLock);
	FNamedOnlineSessionRef NewSession = MakeShared<FNamedOnlineSession, ESPMode::ThreadSafe>(SessionName, SessionSettings);
	Sessions.Emplace(NewSession);
	return &NewSession.Get();
}


FNamedOnlineSessionRef FOnlineSessionPlayFab::AddNamedSessionRef(FName SessionName, const FOnlineSessionSettings& SessionSettings)
{
	FScopeLock ScopeLock(&SessionLock);
	FNamedOnlineSessionRef NewSession = MakeShared<FNamedOnlineSession, ESPMode::ThreadSafe>(SessionName, SessionSettings);
	Sessions.Emplace(NewSession);
	return NewSession;
}

FNamedOnlineSession* FOnlineSessionPlayFab::AddNamedSession(FName SessionName, const FOnlineSession& Session)
{
	FScopeLock ScopeLock(&SessionLock);
	FNamedOnlineSessionRef NewSession = MakeShared<FNamedOnlineSession, ESPMode::ThreadSafe>(SessionName, Session);
	Sessions.Emplace(NewSession);
	return &NewSession.Get();
}

FNamedOnlineSessionRef FOnlineSessionPlayFab::AddNamedSessionRef(FName SessionName, const FOnlineSession& Session)
{
	FScopeLock ScopeLock(&SessionLock);
	FNamedOnlineSessionRef NewSession = MakeShared<FNamedOnlineSession, ESPMode::ThreadSafe>(SessionName, Session);
	Sessions.Emplace(NewSession);
	return NewSession;
}

FNamedOnlineSessionPtr FOnlineSessionPlayFab::GetNamedSessionPtr(FName SessionName) const
{
	FScopeLock ScopeLock(&SessionLock);
	for (const FNamedOnlineSessionRef& Session : Sessions)
	{
		if (Session->SessionName == SessionName)
		{
			return Session;
		}
	}
	return nullptr;
}

const FName FOnlineSessionPlayFab::GetNativeSessionName() const
{
	return NativeSessionName;
}

void FOnlineSessionPlayFab::Tick(float DeltaTime)
{
	RetryJoinMatchmakingSession_TimeSinceLastRetry += DeltaTime;

	if (RetryJoinMatchmakingSession_Count > 0)
	{
		if (RetryJoinMatchmakingSession_TimeSinceLastRetry > RetryJoinMatchmakingSession_RetryTime)
		{
			RetryJoinMatchmakingSession_TimeSinceLastRetry = 0.0f;
			RetryJoinMatchmakingSession_Count -= 1;

			OnOperationComplete_TryJoinNetwork(false, RetryJoinMatchmakingSession_Count);
		}
	}

	RetryJoinLobbySession_TimeSinceLastRetry += DeltaTime;

	if (RetryJoinLobbySession_Count > 0)
	{
		if (RetryJoinLobbySession_TimeSinceLastRetry > RetryJoinLobbySession_RetryTime)
		{
			RetryJoinLobbySession_TimeSinceLastRetry = 0.0f;
			RetryJoinLobbySession_Count -= 1;

			OnOperationComplete_TryJoinNetwork(true, RetryJoinLobbySession_Count);
		}
	}

	OSSPlayFab->GetPlayFabLobbyInterface()->DoWork();

	OSSPlayFab->GetMatchmakingInterface()->DoWork();

	if (bUsesNativeSession)
	{
		OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
		{
			if (!OnNativeSessionUserInviteAcceptedDelegateHandle.IsValid())
			{
				OnNativeSessionUserInviteAcceptedDelegateHandle = NativeSessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(OnNativeSessionUserInviteAcceptedDelegate);
			}
		}
	}

#if defined(OSS_PLAYFAB_GDK)
	TickPendingInvites();
#endif
}

FOnlineSessionSearchResult FOnlineSessionPlayFab::CreateSearchResultFromInvite(const PFLobbyInviteReceivedStateChange& StateChange)
{
	const FString LobbyConnectionString(UTF8_TO_TCHAR(StateChange.connectionString));

	TSharedRef<FOnlineSessionInfoPlayFab> NewSessionInfo = MakeShared<FOnlineSessionInfoPlayFab>();
	NewSessionInfo->ConnectionString = LobbyConnectionString;

	FOnlineSessionSearchResult NewSearchResult;
	NewSearchResult.Session.SessionInfo = NewSessionInfo;
	NewSearchResult.Session.OwningUserId = FUniqueNetIdPlayFab::EmptyId(); // TODO we do not receive the owner, but only the inviting Entity.

	NewSearchResult.Session.SessionSettings.Set(SETTING_CONNECTION_STRING, LobbyConnectionString, EOnlineDataAdvertisementType::ViaOnlineService);

	return NewSearchResult;
}

// TODO remove this function as a part of cleanup
FString FOnlineSessionPlayFab::GetPlatformIdFromEntityId(const FString& EntityId)
{
	FString PlatformUserId;

	IOnlineVoicePtr VoiceIntPtr = OSSPlayFab->GetVoiceInterface();
	if (VoiceIntPtr.IsValid())
	{
		FOnlineVoicePlayFab* PlayFabVoiceInt = static_cast<FOnlineVoicePlayFab*>(VoiceIntPtr.Get());
		if (PlayFabVoiceInt)
		{
			PlatformUserId = PlayFabVoiceInt->GetPlatformIdFromEntityId(EntityId);
		}
	}

	return PlatformUserId;
}

void FOnlineSessionPlayFab::OnCreateSessionCompleted(FName SessionName, bool bPlayFabSessionCreated)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCreateSessionCompleted()"));

	bool bSuccess = false;

	if (bPlayFabSessionCreated)
	{
		if (PendingCreateSessionInfo.PlayerId.IsValid())
		{
			FUniqueNetIdPtr NativeNetId = OSSPlayFab->GetNativeNetId(PendingCreateSessionInfo.PlayerId->AsShared());
			if (NativeNetId.IsValid())
			{
				FNamedOnlineSessionPtr ExistingNamedSession = GetNamedSessionPtr(SessionName);
				if (ExistingNamedSession.IsValid())
				{
					FOnlineSessionInfoPlayFabPtr PlayFabSessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(ExistingNamedSession->SessionInfo);
					if (PlayFabSessionInfo.IsValid())
					{
						ConnectionString = PlayFabSessionInfo->ConnectionString;
						if (ConnectionString.IsEmpty() == false)
						{
							if (bUsesNativeSession)
							{
								OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
								{
									OnNativeCreateSessionCompleteDelegateHandle = NativeSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
										FOnCreateSessionCompleteDelegate::CreateLambda([this, NativeSessionInterface, ExistingNamedSession, SessionName](FName CallbackNativeSessionName, bool bNativeSessionCreated)
										{
											if (bNativeSessionCreated == false)
											{
												OSSPlayFab->LeavePlayFabPartyNetwork();
												OSSPlayFab->GetPlayFabLobbyInterface()->LeaveLobby(*FUniqueNetIdPlayFab::EmptyId(), SessionName, FOnDestroySessionCompleteDelegate(), FOnUnregisterLocalPlayerCompleteDelegate(), true);

												UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeCreateSessionComplete: Failed to create native session, leaving Playfab Party network"));
											}
											else
											{
												FNamedOnlineSession* ExistingNativeNamedSession = NativeSessionInterface->GetNamedSession(CallbackNativeSessionName);
												if (ExistingNativeNamedSession)
												{
													FUniqueNetIdRef NativeSessionId = ExistingNativeNamedSession->SessionInfo->GetSessionId().AsShared();
													if (NativeSessionId->IsValid())
													{
														ExistingNamedSession->SessionSettings.Set(FName(SEARCH_KEY_NATIVE_SESSIONID), NativeSessionId->ToString(), EOnlineDataAdvertisementType::ViaOnlineService);
#if defined(OSS_PLAYFAB_WIN64)
														ExistingNamedSession->SessionSettings.Set(FName(SEARCH_KEY_NATIVE_PLATFORM), PLATFORM_MODEL_WIN64, EOnlineDataAdvertisementType::ViaOnlineService);
#elif defined(OSS_PLAYFAB_PLAYSTATION)
														ExistingNamedSession->SessionSettings.Set(FName(SEARCH_KEY_NATIVE_PLATFORM), PLATFORM_MODEL_SONY, EOnlineDataAdvertisementType::ViaOnlineService);
#endif
														if (!UpdateSession(SessionName, ExistingNamedSession->SessionSettings, true))
														{
															UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeCreateSessionComplete: UpdateSession failed"));
														}
													}

													else
													{
														UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeCreateSessionComplete: NativeSessionId was invalid"));
													}
												}
												else
												{
													UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnNativeCreateSessionComplete: ExistingNativeNamedSession was null"));
												}
											}
											TriggerOnCreateSessionCompleteDelegates(SessionName, bNativeSessionCreated);
											NativeSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnNativeCreateSessionCompleteDelegateHandle);
										}));

									FOnlineSessionSettings SessionSettings = PendingCreateSessionInfo.SessionSettings;
									SessionSettings.Set(SETTING_CONNECTION_STRING, ConnectionString, EOnlineDataAdvertisementType::ViaOnlineService);
									bSuccess = NativeSessionInterface->CreateSession(*NativeNetId, NativeSessionName, SessionSettings);
								}
							}
							else
							{
								bSuccess = true;

								TriggerOnCreateSessionCompleteDelegates(SessionName, bSuccess);
							}
						}
						else
						{
							UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreateSessionCompleted: ConnectionString was invalid"));
						}
					}
					else
					{
						UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreateSessionCompleted: PlayFabSessionInfo was invalid"));
					}
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreateSessionCompleted: ExistingNamedSession was null"));
				}
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreateSessionCompleted: NativeNetId was invalid"));
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreateSessionCompleted: PlayerId was invalid"));
		}
	}
	
	if (bSuccess == false)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreateSessionCompleted: failed to create session, leaving Playfab Party network"));
		OSSPlayFab->LeavePlayFabPartyNetwork();
		TriggerOnCreateSessionCompleteDelegates(SessionName, false);
	}
}

void FOnlineSessionPlayFab::GenerateCrossNetworkVoiceChatPlatformPermissions()
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::GenerateCrossNetworkVoiceChatPlatformPermissions()"));

	VoiceChatPlatforms.Emplace(PLATFORM_MODEL_WINGDK, ECrossNetworkType::GDK);
	VoiceChatPlatforms.Emplace(PLATFORM_MODEL_XSX, ECrossNetworkType::GDK);
	VoiceChatPlatforms.Emplace(PLATFORM_MODEL_XB1, ECrossNetworkType::GDK);
	VoiceChatPlatforms.Emplace(PLATFORM_MODEL_WIN64, ECrossNetworkType::NONGDK);
	VoiceChatPlatforms.Emplace(PLATFORM_MODEL_SWITCH, ECrossNetworkType::NONGDK);
	VoiceChatPlatforms.Emplace(PLATFORM_MODEL_PS4, ECrossNetworkType::NONGDK);
	VoiceChatPlatforms.Emplace(PLATFORM_MODEL_PS5, ECrossNetworkType::NONGDK);
	
	// load all voice chat disabled platforms
	TArray<FString> DisabledPlatforms;
	GConfig->GetArray(TEXT("OnlineSubsystemPlayFabVoiceChatDisabledPlatforms"), TEXT("Platforms"), DisabledPlatforms, GEngineIni);

	for (auto& DisabledPlatform : DisabledPlatforms)
	{
		VoiceChatPlatforms.Emplace(DisabledPlatform, ECrossNetworkType::DISABLED);
	}
}

bool FOnlineSessionPlayFab::IsHostSetting(const FName& Name)
{
	return Name == SETTING_HOST_NICKNAME;
}

void FOnlineSessionPlayFab::OnNativeSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnNativeSessionUserInviteAccepted()"));

	ConnectionString = InviteResult.GetSessionIdStr();
	TriggerOnSessionUserInviteAcceptedDelegates(bWasSuccessful, ControllerId, UserId, InviteResult);
}

#pragma region Helpers
bool FOnlineSessionPlayFab::GetLobbyHandleBySessionName(const FName SessionName, PFLobbyHandle& LobbyHandle)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::GetLobbyHandleBySessionName()"));

	FNamedOnlineSessionPtr SessionPtr = GetNamedSessionPtr(SessionName);
	if (SessionPtr.IsValid())
	{
		FOnlineSessionInfoPlayFabPtr SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(SessionPtr->SessionInfo);
		if (SessionInfo.IsValid())
		{
			LobbyHandle = SessionInfo->LobbyHandle;
			return true;
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::GetLobbyHandleBySessionName: SessionInfo was null"));
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::GetLobbyHandleBySessionName: failed to find the lobby for session %s!"), *SessionName.ToString());
	}

	return false;
}

bool FOnlineSessionPlayFab::ValidateSessionForInvite(const FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::ValidateSessionForInvite()"));

	FNamedOnlineSessionPtr SessionPtr = GetNamedSessionPtr(SessionName);
	if (!SessionPtr.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::ValidateSessionForInvite: Cannot Invite Friends to session %s, that session does not exist"), *SessionName.ToString());
		return false;
	}

	if (SessionPtr->SessionState < EOnlineSessionState::Pending || SessionPtr->SessionState > EOnlineSessionState::InProgress)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::ValidateSessionForInvite: Cannot Invite Friends to session %s, that session is in state %d"), *SessionName.ToString(), SessionPtr->SessionState);
		return false;
	}

	FOnlineSessionInfoPlayFabPtr SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(SessionPtr->SessionInfo);
	if (!SessionInfo.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::ValidateSessionForInvite: Cannot Invite Friends to session %s, that session has invalid info"), *SessionName.ToString());
		return false;
	}

	return true;
}

#pragma endregion

#pragma region PlayFab Multiplayer Invites
bool FOnlineSessionPlayFab::SendSessionInviteToFriend_PlayFabMultiplayer(const PFEntityKey& LocalUserEntityKey, FName SessionName, const PFEntityKey& RemoteUserEntityKey)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SendSessionInviteToFriend_PlayFabMultiplayer()"));

	TArray<PFEntityKey> RemoteUserEntityKeys;
	RemoteUserEntityKeys.Add(RemoteUserEntityKey);

	return SendSessionInviteToFriends_PlayFabMultiplayer(LocalUserEntityKey, SessionName, RemoteUserEntityKeys);
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriends_PlayFabMultiplayer(const PFEntityKey& LocalUserEntityKey, FName SessionName, const TArray<PFEntityKey>& RemoteUserEntityKeys)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SendSessionInviteToFriends_PlayFabMultiplayer()"));

	if (RemoteUserEntityKeys.Num() < 1)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SendInvite_PlayFabMultiplayer: RemoteUserEntityKeys was empty"));
		return true;
	}

	if (!ValidateSessionForInvite(SessionName))
	{
		return false;
	}

	PFLobbyHandle LobbyHandle{};
	if (!GetLobbyHandleBySessionName(SessionName, LobbyHandle))
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SendInvite_PlayFabMultiplayer: No lobby found for session %s!"), *SessionName.ToString());
		return false;
	}

	for (const PFEntityKey& RemoteUserEntityKey : RemoteUserEntityKeys)
	{
		HRESULT Hr = PFLobbySendInvite(LobbyHandle, &LocalUserEntityKey, &RemoteUserEntityKey, nullptr);
		if (FAILED(Hr))
		{
			LogMultiplayerErrorWithMessage("PFLobbySendInvite", Hr);
			return false;
		}
	}

	return true;
}

void FOnlineSessionPlayFab::OnInvitationReceived_PlayFabMultiplayer(const PFEntityKey& ListeningEntityKey, const PFEntityKey& InvitingEntityKey, const FString& InConnectionString)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnInvitationReceived_PlayFabMultiplayer()"));

	TriggerOnPlayFabMultiplayerInviteReceivedDelegates(ListeningEntityKey, InvitingEntityKey, InConnectionString);
}

void FOnlineSessionPlayFab::OnAppResume(FOnSessionsRemovedDelegate& CompletionDelegate)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnAppResume()"));

	if (Sessions.Num() == 0)
	{
		CompletionDelegate.ExecuteIfBound();
		return;
	}

	OnAppResumeDelegate = CompletionDelegate;

	auto OnSessionsRemoved = [&]() {
		OnAppResumeDelegate.ExecuteIfBound();
	};
	OnSessionsRemovedDelegate.BindLambda(OnSessionsRemoved);
}

#pragma endregion