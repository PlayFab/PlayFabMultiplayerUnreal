//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSessionSettings.h"
#include "PlayFabSocketSubsystem.h"
#include "../PlatformSpecific/PlatformDefines.h"

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
#include "OnlineSubsystemGDK.h"
THIRD_PARTY_INCLUDES_START
#include <xsapi-c/multiplayer_activity_c.h>
THIRD_PARTY_INCLUDES_END
#endif

#include "OnlineSubsystemPlayFab.h"
#include "OnlineSubsystemPlayFabPrivate.h"

#define OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();  IOnlineSessionPtr NativeSessionInterface = NativeSubsystem ? NativeSubsystem->GetSessionInterface() : nullptr; if (NativeSessionInterface)

FOnlineSessionPlayFab::FOnlineSessionPlayFab(class FOnlineSubsystemPlayFab* InSubsystem) :
	OSSPlayFab(InSubsystem)
{
	RegisterForUpdates();
}

FOnlineSessionPlayFab::~FOnlineSessionPlayFab()
{
	// Clean up delegates
	if (OSSPlayFab)
	{
		OSSPlayFab->ClearOnConnectToPlayFabPartyNetworkCompletedDelegates(this);
		ClearOnMatchmakingCompleteDelegate_Handle(OnMatchmakingCompleteDelegateHandle);
		UnregisterForUpdates();
	}
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
			UE_LOG_ONLINE_SESSION(Log, TEXT("Removing Session %s"), *SessionName.ToString());
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
	bool bSuccess = false;
	// Check for an existing session
	if (GetNamedSessionPtr(SessionName).IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot create session '%s': session already exists."), *SessionName.ToString());
		OnCreateSessionCompleted(SessionName, false);
		return false;
	}

	PendingCreateSessionInfo.PlayerControllerIndex = HostingPlayerControllerIndex;
	PendingCreateSessionInfo.PlayerId = nullptr;
	PendingCreateSessionInfo.SessionName = SessionName;
	PendingCreateSessionInfo.SessionSettings = NewSessionSettings;

	if (OSSPlayFab)
	{
		OSSPlayFab->AddOnPartyEndpointCreatedDelegate_Handle(FOnPartyEndpointCreatedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCreatePartyEndpoint));

		bSuccess = OSSPlayFab->CreateAndConnectToPlayFabPartyNetwork();
	}

	if (bSuccess == false)
	{
		OnCreateSessionCompleted(SessionName, false);
	}

	return bSuccess;
}

bool FOnlineSessionPlayFab::CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	bool bSuccess = false;
	// Check for an existing session
	if (GetNamedSessionPtr(SessionName).IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot create session '%s': session already exists."), *SessionName.ToString());
		OnCreateSessionCompleted(SessionName, false);
		return false;
	}

	PendingCreateSessionInfo.PlayerControllerIndex = INDEX_NONE;
	PendingCreateSessionInfo.PlayerId = HostingPlayerId.AsShared();
	PendingCreateSessionInfo.SessionName = SessionName;
	PendingCreateSessionInfo.SessionSettings = NewSessionSettings;

	if (OSSPlayFab)
	{
		OSSPlayFab->AddOnPartyEndpointCreatedDelegate_Handle(FOnPartyEndpointCreatedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCreatePartyEndpoint));

		bSuccess = OSSPlayFab->CreateAndConnectToPlayFabPartyNetwork();
	}

	if (bSuccess == false)
	{
		OnCreateSessionCompleted(SessionName, false);
	}

	return bSuccess;
}

void FOnlineSessionPlayFab::OnCreatePartyEndpoint(bool bSuccess, uint16 EndpointID, bool bIsHosting)
{
	if (OSSPlayFab)
	{
		OSSPlayFab->ClearOnPartyEndpointCreatedDelegates(this);
	}

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

				PendingCreateSessionInfo.SessionSettings.bIsDedicated = true;
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
	// Create a new session and deep copy the game settings
	FNamedOnlineSessionRef Session = AddNamedSessionRef(SessionName, NewSessionSettings);
	Session->SessionState = EOnlineSessionState::Creating;
	Session->bHosting = true;
	Session->OwningUserId = FUniqueNetIdPlayFab::Create(HostingPlayerId);
	Session->LocalOwnerId = HostingPlayerId.AsShared();

	if (!OSSPlayFab)
	{
		return false;
	}

	OnLobbyCreatedAndJoinCompletedDelegateHandle = FOnLobbyCreatedAndJoinCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyCreatedAndJoinCompleted);
	OnLobbyCreatedAndJoinCompletedHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyCreatedAndJoinCompletedDelegate_Handle(OnLobbyCreatedAndJoinCompletedDelegateHandle);

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->CreatePlayFabLobby(HostingPlayerId, SessionName, NewSessionSettings))
	{
		UE_LOG_ONLINE_SESSION(Log, TEXT("Failed to create create lobby operation"));
		RemoveNamedSession(SessionName);
		return false;
	}

	return true;
}

void FOnlineSessionPlayFab::OnLobbyCreatedAndJoinCompleted(bool fSuccess, FName SessionName)
{
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyCreatedAndJoinCompletedDelegate_Handle(OnLobbyCreatedAndJoinCompletedHandle);
	OnCreateSessionCompleted(SessionName, fSuccess);
}

bool FOnlineSessionPlayFab::StartSession(FName SessionName)
{
	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	if (!Session.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't start an online game for session (%s) that hasn't been created"),
			*SessionName.ToString());
		TriggerOnStartSessionCompleteDelegates(SessionName, false);
		return false;
	}

	if (Session->SessionState != EOnlineSessionState::Pending &&
		Session->SessionState != EOnlineSessionState::Ended)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't start an online session (%s) in state %s"),
			*SessionName.ToString(),
			EOnlineSessionState::ToString(Session->SessionState));
		TriggerOnStartSessionCompleteDelegates(SessionName, false);
		return false;
	}

	Session->SessionState = EOnlineSessionState::InProgress;

	TriggerOnStartSessionCompleteDelegates(SessionName, true);
	return true;
}

bool FOnlineSessionPlayFab::UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData /*= false*/)
{
	FNamedOnlineSessionPtr NamedSession = GetNamedSessionPtr(SessionName);
	if (!NamedSession.IsValid())
	{
		OSSPlayFab->ExecuteNextTick([this, SessionName]()
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("Unable to update session %s, it does not exist"), *SessionName.ToString());
				TriggerOnUpdateSessionCompleteDelegates(SessionName, false);
			});
		return false;
	}

	EOnlineSessionState::Type SessionState = NamedSession->SessionState;
	if (SessionState <= EOnlineSessionState::Creating || SessionState >= EOnlineSessionState::Destroying)
	{
		OSSPlayFab->ExecuteNextTick([this, SessionName, SessionState]()
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("Unable to update session %s, it is state %s, which may not be updated"), *SessionName.ToString(), EOnlineSessionState::ToString(SessionState));
				TriggerOnUpdateSessionCompleteDelegates(SessionName, false);
			});
		return false;
	}

	OnUpdateLobbyCompleteDelegate = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnUpdateLobbyCompletedDelegate_Handle(FOnUpdateLobbyCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnUpdateLobbyCompleted));
	if (!OSSPlayFab->GetPlayFabLobbyInterface()->UpdateLobby(SessionName, UpdatedSessionSettings))
	{
		UE_LOG_ONLINE_SESSION(Log, TEXT("Failed to update session %s"), *SessionName.ToString());
		OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnUpdateLobbyCompletedDelegate_Handle(OnUpdateLobbyCompleteDelegate);
		return false;
	}

	return true;
}

bool FOnlineSessionPlayFab::EndSession(FName SessionName)
{
	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	if (!Session.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't end an online game for session (%s) that hasn't been created"),
			*SessionName.ToString());
		TriggerOnEndSessionCompleteDelegates(SessionName, false);
		return false;
	}

	if (Session->SessionState != EOnlineSessionState::InProgress)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't end session (%s) in state %s"),
			*SessionName.ToString(),
			EOnlineSessionState::ToString(Session->SessionState));
		TriggerOnEndSessionCompleteDelegates(SessionName, false);
		return false;
	}

	Session->SessionState = EOnlineSessionState::Ended;
	TriggerOnEndSessionCompleteDelegates(SessionName, true);
	return true;
}

bool FOnlineSessionPlayFab::DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate /*= FOnDestroySessionCompleteDelegate()*/)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::DestroySession SessionName:%s"), *SessionName.ToString());
	// Leave the PlayFab Party network
	if (OSSPlayFab)
	{
		OSSPlayFab->LeavePlayFabPartyNetwork();
	}

	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	if (!Session.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't destroy a null online session (%s)"), *SessionName.ToString());
		OSSPlayFab->ExecuteNextTick([this, SessionName, CompletionDelegate]()
		{
			CompletionDelegate.ExecuteIfBound(SessionName, false);
			TriggerOnDestroySessionCompleteDelegates(SessionName, false);
		});
		return false;
	}

	if (Session->SessionState == EOnlineSessionState::Destroying)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Already in process of destroying session (%s)"), *SessionName.ToString());
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
		UE_LOG_ONLINE_SESSION(Log, TEXT("Failed to destroy the session %s"), *SessionName.ToString());
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
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLeaveLobbyCompletedDelegate_Handle(OnLeaveLobbyCompletedHandle);
	TriggerOnDestroySessionCompleteDelegates(SessionName, bSuccess);
}

void FOnlineSessionPlayFab::OnFindLobbiesCompleted(int32 LocalUserNum, bool bSuccess, TSharedPtr<FOnlineSessionSearch> SearchResults)
{
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
		UE_LOG_ONLINE(Error, TEXT("LocalPlayers was empty. At least one player is required for matchmaking."));
		TriggerOnMatchmakingCompleteDelegates(SessionName, false);
		return false;
	}

	OnMatchmakingTicketCompletedDelegateHandle = FOnMatchmakingTicketCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnMatchmakingTicketCompleted);
	OnMatchmakingTicketCompletedHandle = OSSPlayFab->GetMatchmakingInterface()->AddOnMatchmakingTicketCompletedDelegate_Handle(OnMatchmakingTicketCompletedDelegateHandle);

	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::StartMatchmaking:CreateMatchMakingTicket()"));
	if (!OSSPlayFab->GetMatchmakingInterface()->CreateMatchMakingTicket(LocalPlayers, SessionName, NewSessionSettings, SearchSettings))
	{
		UE_LOG_ONLINE_SESSION(Log, TEXT("Failed to CreateMatchMakingTicket"));
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

	FName SessionName;
	if (bJoinLobbyOperation)
	{
		SessionName = JoinSessionCompleteSessionName;
	}
	else
	{
		SessionName = MatchmakingCompleteSessionName;
	}

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

			bool bSuccess = OSSPlayFab->ConnectToPlayFabPartyNetwork(NetworkIdStr, NetworkDescriptorStr);

			if (bSuccess == false)
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

	if (OSSPlayFab)
	{
		OSSPlayFab->ClearOnPartyEndpointCreatedDelegates(this);
	}

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

					SessionSettings->bIsDedicated = true;

					OnUpdateSession_MatchmakingDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnUpdateLobbyCompletedDelegate_Handle(FOnUpdateLobbyCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnUpdateSession_Matchmaking));
					if (!OSSPlayFab->GetPlayFabLobbyInterface()->UpdateLobby(MatchmakingCompleteSessionName, *SessionSettings))
					{
						UE_LOG_ONLINE_SESSION(Error, TEXT("Failed to update Lobby with network descriptor %s"), *MatchmakingCompleteSessionName.ToString());
						OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnUpdateLobbyCompletedDelegate_Handle(OnUpdateSession_MatchmakingDelegateHandle);
						TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
					}
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking: LocalIp was invalid"));
					TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
				}
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking: PlayFabSocketSubsystem was null"));
				TriggerOnMatchmakingCompleteDelegates(MatchmakingCompleteSessionName, false);
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::OnCreatePartyEndpoint_Matchmaking: SessionSettings was null"));
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
	if (OSSPlayFab)
	{
		OSSPlayFab->ClearOnPartyEndpointCreatedDelegates(this);
	}
	
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

	UE_LOG_ONLINE_SESSION(Error, TEXT("FOnlineSessionPlayFab::CancelMatchmaking() SearchingPlayerNum index invalid: %i"), SearchingPlayerNum);
	return false;
}

bool FOnlineSessionPlayFab::CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::CancelMatchmaking()"));

	PFEntityKey key;
	FOnlineIdentityPlayFabPtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityPlayFab>(OSSPlayFab->GetIdentityInterface());
	TSharedPtr<FPlayFabUser> user = IdentityInterface->GetPartyLocalUserFromPlatformId(SearchingPlayerId);
	if (user != nullptr)
	{
		key = user->GetEntityKey();
	}

	OnCancelMatchmakingCompleteDelegateHandle = FOnCancelMatchmakingCompleteDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnCancelMatchmakingComplete);
	OnCancelMatchmakingCompleteHandle = OSSPlayFab->GetMatchmakingInterface()->AddOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegateHandle);

	return OSSPlayFab->GetMatchmakingInterface()->CancelMatchmakingTicket(key, SessionName);
}

bool FOnlineSessionPlayFab::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
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
	OnFindLobbiesCompletedDelegateHandle = FOnFindLobbiesCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnFindLobbiesCompleted);
	OnFindLobbiesCompletedHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnFindLobbiesCompletedDelegate_Handle(OnFindLobbiesCompletedDelegateHandle);

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->FindLobbies(SearchingPlayerId, SearchSettings))
	{
		UE_LOG_ONLINE_SESSION(Error, TEXT("FOnlineSessionPlayFab::FindSessions Failed to FindLobbies"));
		OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnFindLobbiesCompletedDelegate_Handle(OnFindLobbiesCompletedHandle);
		TriggerOnFindSessionsCompleteDelegates(false);
		return false;
	}

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
	UE_LOG_ONLINE_SESSION(Display, TEXT("FOnlineSessionPlayFab::PingSearchResults is not implemented"));
	return false;
}

void FOnlineSessionPlayFab::RegisterForUpdates()
{
	// Register for changes for this session
	OnLobbyUpdateDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyUpdateDelegate_Handle(FOnLobbyUpdateDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyUpdate));
	OnLobbyMemberAddedDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyMemberAddedDelegate_Handle(FOnLobbyMemberAddedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyMemberAdded));
	OnLobbyMemberRemovedDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyMemberRemovedDelegate_Handle(FOnLobbyMemberRemovedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyMemberRemoved));
	OnInvitationReceivedDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnInvitationReceivedDelegate_Handle(FOnInvitationReceivedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnInvitationReceived));
	OnLobbyDisconnectedDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnLobbyDisconnectedDelegate_Handle(FOnLobbyDisconnectedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnLobbyDisconnected));
}

void FOnlineSessionPlayFab::UnregisterForUpdates()
{
	// clear session delegates
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyUpdateDelegate_Handle(OnLobbyUpdateDelegateHandle);
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyMemberAddedDelegate_Handle(OnLobbyMemberAddedDelegateHandle);
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyMemberRemovedDelegate_Handle(OnLobbyMemberRemovedDelegateHandle);
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnInvitationReceivedDelegate_Handle(OnInvitationReceivedDelegateHandle);
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnLobbyDisconnectedDelegate_Handle(OnLobbyDisconnectedDelegateHandle);
}

void FOnlineSessionPlayFab::OnLobbyUpdate(FName SessionName, const PFLobbyUpdatedStateChange& StateChange)
{
	HRESULT Hr;
	FNamedOnlineSessionPtr ExistingNamedSession = GetNamedSessionPtr(SessionName);
	if (!ExistingNamedSession.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate No session found with SessionName:%s!"), *(SessionName.ToString()));
		return;
	}

	for (uint32 i = 0; i < StateChange.updatedLobbyPropertyCount; ++i)
	{
		const char* updatedLobbyKey = StateChange.updatedLobbyPropertyKeys[i];
		const char* updatedLobbyPropertyValue;
		Hr = PFLobbyGetLobbyProperty(StateChange.lobby, updatedLobbyKey, &updatedLobbyPropertyValue);
		if (FAILED(Hr))
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate failed to get lobby property value. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
			continue;
		}

		if (updatedLobbyPropertyValue == nullptr)
		{
			UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Remove Key:%s"), *FString(updatedLobbyKey));
			ExistingNamedSession->SessionSettings.Remove(FName(updatedLobbyKey));
		}
		else if (FString(updatedLobbyKey) == TEXT("_flags"))
		{
			FString SessionSettingsFlagsValue(updatedLobbyPropertyValue);
			int16 SessionSettingsFlags = 0;
			LexFromString(SessionSettingsFlags, *SessionSettingsFlagsValue);

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
			UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Lobby Key:%s, value:%s"), *FString(updatedLobbyKey), *FString(updatedLobbyPropertyValue));
			ExistingNamedSession->SessionSettings.Set(FName(updatedLobbyKey), FString(updatedLobbyPropertyValue), EOnlineDataAdvertisementType::ViaOnlineService);
		}
	}

	for (uint32 i = 0; i < StateChange.updatedSearchPropertyCount; ++i)
	{
		const char* updatedSearchKey = StateChange.updatedSearchPropertyKeys[i];
		const char* updatedSearchPropertyValue;
		Hr = PFLobbyGetSearchProperty(StateChange.lobby, updatedSearchKey, &updatedSearchPropertyValue);
		if (FAILED(Hr))
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate failed to get search property value. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
			continue;
		}

		if (updatedSearchPropertyValue == nullptr)
		{
			UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Remove Key:%s"), *FString(updatedSearchKey));
			ExistingNamedSession->SessionSettings.Remove(FName(updatedSearchKey));
		}
		else
		{
			UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Search Key:%s, value:%s"), *FString(updatedSearchKey), *FString(updatedSearchPropertyValue));
			ExistingNamedSession->SessionSettings.Set(FName(updatedSearchKey), FString(updatedSearchPropertyValue), EOnlineDataAdvertisementType::ViaOnlineService);
		}
	}

	if (StateChange.ownerUpdated)
	{
		const PFEntityKey* OwnerEntityKeyPtr;
		Hr = PFLobbyGetOwner(StateChange.lobby, &OwnerEntityKeyPtr);
		if (SUCCEEDED(Hr))
		{
			if (OwnerEntityKeyPtr != nullptr)
			{
				if (!SetHostOnSession(SessionName, *OwnerEntityKeyPtr))
				{
					UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate invalid owner!"));
				}
			}
			else
			{
				UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate owner is removed from the session!"));
			}
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate failed to GetOwner for the lobby. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		}
	}

	if (StateChange.maxMembersUpdated)
	{
		uint32 NewMemberCount;
		Hr = PFLobbyGetMaxMemberCount(StateChange.lobby, &NewMemberCount);
		if (SUCCEEDED(Hr))
		{
			UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate max member count updated to :%u"), NewMemberCount);
			ExistingNamedSession->SessionSettings.NumPublicConnections = NewMemberCount;
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate failed to GetMaxMemberCount for the lobby. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		}
	}

	if (StateChange.accessPolicyUpdated)
	{
		PFLobbyAccessPolicy NewAccessPolicy;
		Hr = PFLobbyGetAccessPolicy(StateChange.lobby, &NewAccessPolicy);
		if (SUCCEEDED(Hr))
		{
			UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate access policy updated to :%u"), NewAccessPolicy);
			if (NewAccessPolicy == PFLobbyAccessPolicy::Public)
			{
				ExistingNamedSession->SessionSettings.bShouldAdvertise = true;
			}
			else //(NewAccessPolicy == PFLobbyAccessPolicy::Private) || (NewAccessPolicy == PFLobbyAccessPolicy::Friends)
			{
				ExistingNamedSession->SessionSettings.bShouldAdvertise = false;
			}			
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate failed to GetAccessPolicy for the lobby. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		}
	}

	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate member update count:%u"), StateChange.memberUpdateCount);
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
				MemberSettings = &ExistingNamedSession->SessionSettings.MemberSettings.Add(MakeShared<FUniqueNetIdPlayFab>(*PlatformId), FSessionSettings());
			}
			for (uint32_t j = 0; j < MemberUpdate.updatedMemberPropertyCount; ++j)
			{
				const char* Key = MemberUpdate.updatedMemberPropertyKeys[j];
				const char* Value;
				Hr = PFLobbyGetMemberProperty(StateChange.lobby, MemberEntity, Key, &Value);
				if (SUCCEEDED(Hr))
				{
					MemberSettings->Add(FName(Key), FOnlineSessionSetting(Value != nullptr ? FString(Value) : FString(), EOnlineDataAdvertisementType::ViaOnlineService));
					UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate Update member property for:%s, Key:%s Value:%s"), *FString(MemberEntity->id), *FString(Key), *FString(Value));
				}
			}
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyUpdate PlatformId not found for Entity:%s!"), *FString(MemberEntity->id));
		}
	}

	TriggerOnSessionCustomDataChangedDelegates(SessionName, ExistingNamedSession->SessionSettings);
}

void FOnlineSessionPlayFab::OnLobbyMemberAdded(FName SessionName, const PFLobbyMemberAddedStateChange& StateChange)
{
	const PFEntityKey& MemberAddedEntity = StateChange.member;
	const char* PlatformIdValue;

	HRESULT Hr = PFLobbyGetMemberProperty(StateChange.lobby, &MemberAddedEntity, TCHAR_TO_UTF8(*SETTING_PLATFORM_ID), &PlatformIdValue);
	if (SUCCEEDED(Hr))
	{
		UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyMemberAdded Id:%s, Type:%s PlatformId:%s"), *FString(MemberAddedEntity.id), *FString(MemberAddedEntity.type), *FString(PlatformIdValue));
		EntityPlatformIdMapping.Add(FString(MemberAddedEntity.id), FString(PlatformIdValue));

		const TSharedRef<const FUniqueNetId> MemberAddedNetId = FUniqueNetIdPlayFab::Create(FString(PlatformIdValue));
	TriggerOnSessionParticipantsChangeDelegates(SessionName, *MemberAddedNetId, true);
}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::OnLobbyMemberAdded failed to get PlatformId for Entity:%s!"), *FString(MemberAddedEntity.id));
	}
}

void FOnlineSessionPlayFab::OnLobbyMemberRemoved(FName SessionName, const PFLobbyMemberRemovedStateChange& StateChange)
{
	const PFEntityKey& MemberRemovedEntity = StateChange.member;
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyMemberRemoved Id:%s, Type:%s"), *FString(MemberRemovedEntity.id), *FString(MemberRemovedEntity.type));

	//TODO we have to use a PlatformId here
	const TSharedRef<const FUniqueNetId> MemberRemovedNetId = FUniqueNetIdPlayFab::Create(FString(MemberRemovedEntity.id));
	TriggerOnSessionParticipantsChangeDelegates(SessionName, *MemberRemovedNetId, false);
}

void FOnlineSessionPlayFab::OnInvitationReceived(const PFLobbyInviteReceivedStateChange& StateChange)
{
	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (IdentityIntPtr.IsValid())
	{
		FOnlineSessionSearchResult SearchResult = CreateSearchResultFromInvite(StateChange);
		FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
		FString PlatformUserIdStr = PlayFabIdentityInt->GetPartyLocalUserFromEntityIdString(FString(StateChange.listeningEntity.id))->GetPlatformUserId();

		const int32 ControllerIndex = 0; // TODO
		const IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();
		if (NativeSubsystem == nullptr)
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineIdentityPlayFab::OnInvitationReceived: Native subsystem not found."));
			TriggerOnSessionUserInviteAcceptedDelegates(false, ControllerIndex, nullptr, SearchResult);
			return;
		}

		const IOnlineIdentityPtr NativeIdentityInterface = NativeSubsystem->GetIdentityInterface();
		if (NativeIdentityInterface == nullptr || !NativeIdentityInterface.IsValid())
		{
			UE_LOG_ONLINE(Error, TEXT("FOnlineIdentityPlayFab::OnInvitationReceived: Native identity interface not found."));
			TriggerOnSessionUserInviteAcceptedDelegates(false, ControllerIndex, nullptr, SearchResult);
			return;
		}

		const FUniqueNetIdPtr UniqueNetId = NativeIdentityInterface->CreateUniquePlayerId(PlatformUserIdStr);
		TriggerOnSessionUserInviteAcceptedDelegates(true, ControllerIndex, UniqueNetId, SearchResult);
	}
}

void FOnlineSessionPlayFab::OnLobbyDisconnected(FName SessionName)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyDisconnected SessionName:%s"), *(SessionName.ToString()));

	FNamedOnlineSessionPtr Session = GetNamedSessionPtr(SessionName);
	// if session is still valid then disconnect was due to connection failure
	if (Session.IsValid())
	{
		// Leave the PlayFab Party network
		if (OSSPlayFab)
		{
			OSSPlayFab->LeavePlayFabPartyNetwork();
		}

		RemoveNamedSession(SessionName);
		TriggerOnSessionFailureDelegates(*FUniqueNetIdPlayFab::EmptyId(), ESessionFailure::ServiceConnectionLost);
		UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::OnLobbyDisconnected SessionName:%s disconnected due to ServiceConnectionLost"), *(SessionName.ToString()));
	}
}

bool FOnlineSessionPlayFab::SetHostOnSession(FName SessionName, const PFEntityKey& HostEntityKey)
{
	FNamedOnlineSessionPtr ExistingNamedSession = GetNamedSessionPtr(SessionName);
	if (!ExistingNamedSession.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::SetHostOnSession No session found with SessionName: %s!"), *(SessionName.ToString()));
		return false;
	}

	if (FCStringAnsi::Strcmp(HostEntityKey.type, ENTITY_TYPE_TITLE_PLAYER) != 0)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSessionPlayFab::SetHostOnSession owner type is not title_player_account: %s!"), *(FString(HostEntityKey.type)));
	}

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::SetHostOnSession Identity Interface is invalid"));
		return false;
	}	

	FString* HostPlatformId = EntityPlatformIdMapping.Find(FString(HostEntityKey.id));
	if (HostPlatformId == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::SetHostOnSession couldn't find a new lobby owner in the entity mapping cache!"));
		return false;
	}

	ExistingNamedSession->OwningUserId = FUniqueNetIdPlayFab::Create(*HostPlatformId);
	ExistingNamedSession->OwningUserName = *HostPlatformId; // TODO we want to use a gamertag for OwningUserName	

	UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::SetHostOnSession set host updated:%s"), *ExistingNamedSession->OwningUserId->ToString());

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformIdString(*HostPlatformId);

	if (LocalUser == nullptr)
	{
		UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::SetHostOnSession Identity Interface's GetPartyLocalUserFromPlatformId returned empty user. Local user is not the host."));
	}
	else
	{
		ExistingNamedSession->bHosting = true;
		ExistingNamedSession->LocalOwnerId = ExistingNamedSession->OwningUserId->AsShared();
	}

	return true;
}

void FOnlineSessionPlayFab::SetMultiplayerActivity(const FName SessionName, PFLobbyHandle LobbyHandle)
{
	const char* connectionString;
	HRESULT Hr = PFLobbyGetConnectionString(LobbyHandle, &connectionString);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivity failed to get PFLobbyGetConnectionString with ErrorCode=[0x%08x], Error message:%s for session %s!"), Hr, *GetMultiplayerErrorMessage(Hr), *(SessionName.ToString()));
		return;
	}

	uint32_t MaxPlayerCount;
	Hr = PFLobbyGetMaxMemberCount(LobbyHandle, &MaxPlayerCount);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivity failed to get PFLobbyGetMaxMemberCount with ErrorCode=[0x%08x], Error message:%s for session %s!"), Hr, *GetMultiplayerErrorMessage(Hr), *(SessionName.ToString()));
		return;
	}

	uint32_t CurrentPlayerCount;
	const PFEntityKey* Players;
	Hr = PFLobbyGetMembers(LobbyHandle, &CurrentPlayerCount, &Players);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivity failed to get PFLobbyGetMembers with ErrorCode=[0x%08x], Error message:%s for session %s!"), Hr, *GetMultiplayerErrorMessage(Hr), *(SessionName.ToString()));
		return;
	}

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	FNamedOnlineSessionPtr NamedSession = GetNamedSessionPtr(SessionName);	
	FOnlineSubsystemGDK* GDKSubsystem = static_cast<FOnlineSubsystemGDK*>(IOnlineSubsystem::Get(GDK_SUBSYSTEM));
	if (!GDKSubsystem)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivity GDK_SUBSYSTEM not found for session!"), *(SessionName.ToString()));
		return;
	}

	FGDKContextHandle GDKContext = GDKSubsystem->GetGDKContext(*NamedSession->LocalOwnerId);

	TUniquePtr<XAsyncBlock> pNewAsyncBlock = MakeUnique<XAsyncBlock>();
	pNewAsyncBlock->queue = FGDKAsyncTaskQueue::GetGenericQueue();

	pNewAsyncBlock->callback = [](XAsyncBlock* ab)
	{
		TUniquePtr<XAsyncBlock> AsyncBlock{ ab }; // take ownership of XAsyncBlock
		HRESULT Hr = XAsyncGetStatus(ab, false);
		UE_LOG_ONLINE(Verbose, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivity result 0x%08x"), Hr);
	};

	XblMultiplayerActivityInfo MultiplayerActivityInfo{};
	MultiplayerActivityInfo.connectionString = connectionString;
	MultiplayerActivityInfo.joinRestriction = XblMultiplayerActivityJoinRestriction::Followed;
	MultiplayerActivityInfo.maxPlayers = MaxPlayerCount;
	MultiplayerActivityInfo.currentPlayers = CurrentPlayerCount;

	Hr = XblMultiplayerActivitySetActivityAsync(GDKContext, &MultiplayerActivityInfo, false, pNewAsyncBlock.Get());
	if (SUCCEEDED(Hr))
	{
		pNewAsyncBlock.Release();
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivity XblMultiplayerActivitySetActivityAsync failed with ErrorCode=[0x%08x] for session %s!"), Hr, *(SessionName.ToString()));
	}
#endif //OSS_PLAYFAB_WINGDK || OSS_PLAYFAB_XSX || OSS_PLAYFAB_XBOXONEGDK
}


bool FOnlineSessionPlayFab::JoinSession_PlayFabInternal(int32 ControllerIndex, TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	if (OSSPlayFab)
	{
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
				UE_LOG_ONLINE_SESSION(Warning, TEXT("Join session failed; session (%s) already exists, can't join twice"), *SessionName.ToString());
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("Join session failed; already in session of type %s, you must leave session %s before joining %s"), *SessionName.ToString(), *ExistingSessionId, *NewSessionId);
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
			UE_LOG_ONLINE_SESSION(Log, TEXT("Failed to JoinLobby"));
			RemoveNamedSession(SessionName);
			TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::UnknownError);
			return false;
		}
	}

	return true;
}

bool FOnlineSessionPlayFab::JoinSession(int32 ControllerIndex, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	bool bSuccess = false;

	if (OSSPlayFab)
	{
		bSuccess = JoinSession_PlayFabInternal(ControllerIndex, nullptr, SessionName, DesiredSession);
	}

	if (bSuccess == false)
	{
		TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::UnknownError);
	}

	return bSuccess;
}

bool FOnlineSessionPlayFab::JoinSession(const FUniqueNetId& UserId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	bool bSuccess = false;

	if (OSSPlayFab)
	{
		bSuccess = JoinSession_PlayFabInternal(INDEX_NONE, UserId.AsShared(), SessionName, DesiredSession);
	}

	if (bSuccess == false)
	{
		TriggerOnJoinSessionCompleteDelegates(SessionName, EOnJoinSessionCompleteResult::UnknownError);
	}

	return bSuccess;
}

void FOnlineSessionPlayFab::OnJoinLobbyCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	ClearOnJoinSessionCompleteDelegate_Handle(OnJoinLobbyCompleteDelegateHandle);

	JoinSessionCompleteSessionName = InSessionName;

	//kick off the join logic and give time for the session update
	RetryJoinLobbySession_Count = RetryJoinLobbySession_MaxCount;

	if (Result != EOnJoinSessionCompleteResult::Success && Result != EOnJoinSessionCompleteResult::AlreadyInSession)
	{
		// Leave the PlayFab Party network
		if (OSSPlayFab)
		{
			OSSPlayFab->LeavePlayFabPartyNetwork();
		}
		TriggerOnJoinSessionCompleteDelegates(InSessionName, Result);
	}
}

bool FOnlineSessionPlayFab::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
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
	OnFindFriendLobbiesCompletedHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnFindLobbiesCompletedDelegate_Handle(FOnFindLobbiesCompletedDelegate::CreateRaw(this, &FOnlineSessionPlayFab::OnFindFriendLobbiesCompleted));

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->FindFriendLobbies(LocalUserId))
	{
		UE_LOG_ONLINE_SESSION(Error, TEXT("FOnlineSessionPlayFab::FindFriendSession Failed!"));
		OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnFindLobbiesCompletedDelegate_Handle(OnFindFriendLobbiesCompletedHandle);
		TriggerOnFindFriendSessionCompleteDelegates(-1, false, TArray<FOnlineSessionSearchResult>());
		return false;
	}

	return true;
}

bool FOnlineSessionPlayFab::FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<TSharedRef<const FUniqueNetId>>& FriendList)
{
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
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnFindLobbiesCompletedDelegate_Handle(OnFindFriendLobbiesCompletedHandle);
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, bSuccess, SearchResults->SearchResults);
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	TArray< TSharedRef<const FUniqueNetId> > Friends;
	Friends.Add(FUniqueNetIdPlayFab::Create(Friend));

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

	return OSSPlayFab->GetPlayFabLobbyInterface()->SendInvite(*PlayerId, SessionName, Friends);
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend)
{
	TArray< TSharedRef<const FUniqueNetId> > Friends;
	Friends.Add(FUniqueNetIdPlayFab::Create(Friend));

	return OSSPlayFab->GetPlayFabLobbyInterface()->SendInvite(LocalUserId, SessionName, Friends);
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
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

	if (!OSSPlayFab->GetPlayFabLobbyInterface()->SendInvite(*PlayerId, SessionName, Friends))
	{
		return false;
	}

	return true;
}

bool FOnlineSessionPlayFab::SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends)
{
	return OSSPlayFab->GetPlayFabLobbyInterface()->SendInvite(LocalUserId, SessionName, Friends);
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
	if (!MySession.IsValid())
	{
		return nullptr;
	}

	return &MySession->SessionSettings;
}

bool FOnlineSessionPlayFab::RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)
{
	TArray<TSharedRef<const FUniqueNetId>> Players;
	Players.Add(FUniqueNetIdPlayFab::Create(PlayerId));
	return RegisterPlayers(SessionName, Players, bWasInvited);	
}

bool FOnlineSessionPlayFab::RegisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players, bool bWasInvited /*= false*/)
{
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
								UE_LOG_ONLINE_SESSION(Log, TEXT("Marking Player %s as registered in session %s"), *Players[PlayerIdx]->ToDebugString(), *SessionName.ToString());
							}
						}
					}
					RegisterVoice(*PlayerId);
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Log, TEXT("Player %s already registered in session %s"), *Players[PlayerIdx]->ToDebugString(), *SessionName.ToString());
				}
			}
			bSuccess = true;
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("No session info to join for session (%s)"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("No game present to join for session (%s)"), *SessionName.ToString());
	}

	TriggerOnRegisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

void FOnlineSessionPlayFab::RegisterVoice(const FUniqueNetId& PlayerId)
{
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
	TArray<TSharedRef<const FUniqueNetId>> Players;
	Players.Add(FUniqueNetIdPlayFab::Create(PlayerId));
	return UnregisterPlayers(SessionName, Players);
}

bool FOnlineSessionPlayFab::UnregisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players)
{
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
					TriggerOnSessionParticipantsChangeDelegates(SessionName, *PlayerId, false);
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Warning, TEXT("Player %s is not part of session (%s)"), *PlayerId->ToDebugString(), *SessionName.ToString());
				}
			}

			bSuccess = true;
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("No session info to leave for session (%s)"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("No game present to leave for session (%s)"), *SessionName.ToString());
	}

	TriggerOnUnregisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

void FOnlineSessionPlayFab::UnregisterVoice(const FUniqueNetId& PlayerId)
{
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
	if (!OSSPlayFab->GetPlayFabLobbyInterface()->AddLocalPlayer(PlayerId, SessionName, Delegate))
	{
		UE_LOG_ONLINE_SESSION(Log, TEXT("Failed to add player to session"));
		Delegate.ExecuteIfBound(PlayerId, EOnJoinSessionCompleteResult::UnknownError);
	}
}

void FOnlineSessionPlayFab::UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate)
{
	if (!OSSPlayFab->GetPlayFabLobbyInterface()->LeaveLobby(PlayerId, SessionName, FOnDestroySessionCompleteDelegate(), Delegate, false))
	{
		UE_LOG_ONLINE_SESSION(Log, TEXT("Failed to unregister player from the session session"));
		Delegate.ExecuteIfBound(PlayerId, false);
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

	if (OSSPlayFab)
	{
		OSSPlayFab->GetPlayFabLobbyInterface()->DoWork();

		OSSPlayFab->GetMatchmakingInterface()->DoWork();
	}
}

FOnlineSessionSearchResult FOnlineSessionPlayFab::CreateSearchResultFromInvite(const PFLobbyInviteReceivedStateChange& StateChange)
{
	FOnlineSessionSearchResult NewSearchResult;
	NewSearchResult.Session.SessionInfo = MakeShared<FOnlineSessionInfoPlayFab>(FString(StateChange.connectionString));
	// TODO we do not receive the owner, but only the inviting Entity.
	NewSearchResult.Session.OwningUserId = FUniqueNetIdPlayFab::Create("");

	NewSearchResult.Session.SessionSettings.Set(SETTING_CONNECTION_STRING, FString(StateChange.connectionString), EOnlineDataAdvertisementType::ViaOnlineService);

	return NewSearchResult;
}

// TODO remove this function as a part of cleanup
FString FOnlineSessionPlayFab::GetPlatformIdFromEntityId(const FString& EntityId)
{
	FString PlatformUserId;

	if (OSSPlayFab)
	{
		IOnlineVoicePtr VoiceIntPtr = OSSPlayFab->GetVoiceInterface();
		if (VoiceIntPtr.IsValid())
		{
			FOnlineVoicePlayFab* PlayFabVoiceInt = static_cast<FOnlineVoicePlayFab*>(VoiceIntPtr.Get());
			if (PlayFabVoiceInt)
			{
				PlatformUserId = PlayFabVoiceInt->GetPlatformIdFromEntityId(EntityId);
			}
		}
	}

	return PlatformUserId;
}

void FOnlineSessionPlayFab::OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful == false)
	{
		// Leave the PlayFab Party network
		if (OSSPlayFab)
		{
			OSSPlayFab->LeavePlayFabPartyNetwork();
		}
	}

	TriggerOnCreateSessionCompleteDelegates(SessionName, bWasSuccessful);
}