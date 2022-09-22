//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemPlayFabDefines.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "PlayFabLobby.h"
#include "MatchmakingInterfacePlayFab.h"

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
#include "OnlineSubsystemGDKTypes.h"
#endif

THIRD_PARTY_INCLUDES_START
#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_PLAYSTATION)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH || OSS_PLAYFAB_PLAYSTATION
#include <PFEntityKey.h>
#include <PFMultiplayer.h>
#include <PFLobby.h>
THIRD_PARTY_INCLUDES_END

using FNamedOnlineSessionRef = TSharedRef<FNamedOnlineSession, ESPMode::ThreadSafe>;
using FNamedOnlineSessionPtr = TSharedPtr<FNamedOnlineSession, ESPMode::ThreadSafe>;

class FInternetAddr;
class FOnlineSettings;
class FUniqueNetIdPlayFab;

struct FPendingCreateSessionInfo
{
	int32 PlayerControllerIndex = INDEX_NONE;
	TSharedPtr<const FUniqueNetId> PlayerId;
	FName SessionName;
	FOnlineSessionSettings SessionSettings;
};

struct FPendingJoinSessionInfo
{
	int32 PlayerControllerIndex = INDEX_NONE;
	TSharedPtr<const FUniqueNetId> PlayerId;
	FName SessionName;
	FOnlineSessionSearchResult SessionSearchResult;
};

class FOnlineSessionPlayFab : public IOnlineSession
{
public:
	FOnlineSessionPlayFab(class FOnlineSubsystemPlayFab* InSubsystem);
	virtual ~FOnlineSessionPlayFab();

	/** Performs common constructor operations. */
	void Initialize();

	// IOnlineSession interface
	virtual TSharedPtr<const FUniqueNetId> CreateSessionIdFromString(const FString& SessionIdStr) override;
	virtual FNamedOnlineSession* GetNamedSession(FName SessionName) override;

	virtual void RemoveNamedSession(FName SessionName) override;
	virtual bool HasPresenceSession() override;
	virtual EOnlineSessionState::Type GetSessionState(FName SessionName) const override;
	virtual bool CreateSession(int32 HostingPlayerControllerIndex, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool StartSession(FName SessionName) override;
	virtual bool UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData = false)  override;
	virtual bool EndSession(FName SessionName) override;
	virtual bool DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate()) override;
	virtual bool IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId) override;
	virtual bool StartMatchmaking(const TArray< TSharedRef<const FUniqueNetId> >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName) override;
	virtual bool CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName) override;
	virtual bool FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegate) override;
	virtual bool CancelFindSessions() override;
	virtual bool PingSearchResults(const FOnlineSessionSearchResult& SearchResult) override;
	virtual bool JoinSession(int32 ControllerIndex, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool JoinSession(const FUniqueNetId& UserId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<TSharedRef<const FUniqueNetId>>& FriendList) override;
	virtual bool SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends) override;
	virtual bool SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Friends) override;
	virtual bool GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType)  override;
	virtual bool GetResolvedConnectString(const class FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo)  override;
	virtual FOnlineSessionSettings* GetSessionSettings(FName SessionName) override;
	virtual bool RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)  override;
	virtual bool RegisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players, bool bWasInvited = false)  override;
	virtual bool UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId)  override;
	virtual bool UnregisterPlayers(FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& Players)  override;
	virtual void RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual void UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual int32 GetNumSessions() override;
	virtual void DumpSessionState() override;

	// IOnlineSession interface
	FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSessionSettings& SessionSettings) override;
	FNamedOnlineSessionRef AddNamedSessionRef(FName SessionName, const FOnlineSessionSettings& SessionSettings);
	FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSession& Session) override;
	FNamedOnlineSessionRef AddNamedSessionRef(FName SessionName, const FOnlineSession& Session);

	FNamedOnlineSessionPtr GetNamedSessionPtr(FName SessionName) const;

	void RegisterForUpdates();
	void UnregisterForUpdates();

	FDelegateHandle OnLobbyUpdateDelegateHandle, OnLobbyMemberAddedDelegateHandle, OnLobbyMemberRemovedDelegateHandle, OnInvitationReceivedDelegateHandle, OnLobbyDisconnectedDelegateHandle;
	void OnLobbyUpdate(FName SessionName, const PFLobbyUpdatedStateChange& StateChange);
	void OnLobbyMemberAdded(FName SessionName, const PFLobbyMemberAddedStateChange& StateChange);
	void OnLobbyMemberRemoved(FName SessionName, const PFLobbyMemberRemovedStateChange& StateChange);
	void OnInvitationReceived(const PFLobbyInviteReceivedStateChange& StateChange);
	void OnLobbyDisconnected(FName SessionName);
	bool SetHostOnSession(FName SessionName, const PFEntityKey& HostEntityKey);

	void SetMultiplayerActivity(const FName SessionName, PFLobbyHandle lobby) const;
	void DeleteMultiplayerActivity(const FName SessionName) const;

PACKAGE_SCOPE:
	/** Critical sections for thread safe operation of session lists */
	mutable FCriticalSection SessionLock;

	/** Current search object */
	TSharedPtr<FOnlineSessionSearch> CurrentSessionSearch;

	/** Current session settings */
	TArray<FNamedOnlineSessionRef> Sessions;

protected:
	bool JoinSession_PlayFabInternal(int32 ControllerIndex, TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession);

	void OnCreatePartyEndpoint(bool bSuccess, uint16 EndpointID, bool bIsHosting);

	class FOnlineSubsystemPlayFab* OSSPlayFab = nullptr;

	FPendingCreateSessionInfo PendingCreateSessionInfo;

	FName JoinSessionCompleteSessionName;
	FDelegateHandle OnJoinLobbyCompleteDelegateHandle;
	void OnJoinLobbyCompleted(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);

	FName MatchmakingCompleteSessionName;
	FDelegateHandle OnMatchmakingCompleteDelegateHandle;
	void OnMatchmakingComplete(FName SessionName, bool bWasSuccessful);

	FDelegateHandle OnUpdateSession_MatchmakingDelegateHandle, OnUpdateLobbyCompleteDelegate;
	void OnUpdateSession_Matchmaking(FName SessionName, bool bWasSuccessful);
	void OnUpdateLobbyCompleted(FName SessionName, bool bWasSuccessful);

	float RetryJoinMatchmakingSession_RetryTime = 2.0f;
	float RetryJoinMatchmakingSession_TimeSinceLastRetry = 0.0f;
	int32 RetryJoinMatchmakingSession_Count = 0;
	int32 RetryJoinMatchmakingSession_MaxCount = 15;

	float RetryJoinLobbySession_RetryTime = 2.0f;
	float RetryJoinLobbySession_TimeSinceLastRetry = 0.0f;
	int32 RetryJoinLobbySession_Count = 0;
	int32 RetryJoinLobbySession_MaxCount = 15;

	void OnOperationComplete_TryJoinNetwork(bool bJoinLobbyOperation, int32& RetryOperationSession_Count);

	void OnCreatePartyEndpoint_Matchmaking(bool bSuccess, uint16 EndpointID, bool bIsHosting);
	void OnCreatePartyEndpoint_JoinSession(bool bSuccess, uint16 EndpointID, bool bIsHosting);

public:
	void Tick(float DeltaTime);

	void OnCreateSessionCompleted(FName SessionName, bool bWasSuccessful);	

	void OnLobbyCreatedAndJoinCompleted(bool bSuccess, FName SessionName);
	FOnLobbyCreatedAndJoinCompletedDelegate OnLobbyCreatedAndJoinCompletedDelegateHandle;
	FDelegateHandle OnLobbyCreatedAndJoinCompletedHandle;

	void OnMatchmakingTicketCompleted(bool bSuccess, FName SessionName);
	FOnMatchmakingTicketCompletedDelegate OnMatchmakingTicketCompletedDelegateHandle;
	FDelegateHandle OnMatchmakingTicketCompletedHandle;

	void OnCancelMatchmakingComplete(FName SessionName, bool bSuccess);
	FOnCancelMatchmakingCompleteDelegate OnCancelMatchmakingCompleteDelegateHandle;
	FDelegateHandle OnCancelMatchmakingCompleteHandle;

	void OnLeaveLobbyCompleted(FName SessionName, bool bSuccess);
	FDelegateHandle OnLeaveLobbyCompletedHandle;

	void OnFindLobbiesCompleted(int32 LocalUserNum, bool bSuccess, TSharedPtr<FOnlineSessionSearch> SearchResults);
	FOnFindLobbiesCompletedDelegate OnFindLobbiesCompletedDelegateHandle;
	FDelegateHandle OnFindLobbiesCompletedHandle;

	void OnFindFriendLobbiesCompleted(int32 LocalUserNum, bool bSuccess, TSharedPtr<FOnlineSessionSearch> SearchResults);
	FDelegateHandle OnFindFriendLobbiesCompletedHandle;

	#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	FGDKContextHandle GetGDKContextSample(const FName SessionName) const;
	#endif

private:
	bool InternalCreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings);
	void RegisterVoice(const FUniqueNetId& PlayerId);
	void UnregisterVoice(const FUniqueNetId& PlayerId);
	FString GetPlatformIdFromEntityId(const FString& EntityId);
	FOnlineSessionSearchResult CreateSearchResultFromInvite(const PFLobbyInviteReceivedStateChange& StateChange);

	#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	void RecordRecentlyMetPlayer(const PFEntityKey& MemberAddedEntity, const FName SessionName, const char* PlatformIdValue) const;
	XTaskQueueRegistrationToken InviteAcceptedHandler = { 0 };
	void SaveInviteFromActivation(void* Context, const char* InviteUri);
	void SaveInviteSession(const int32 ControllerIndex,
						   const FOnlineSessionSearchResult & SearchResult);
	void TickPendingGDKInvites();
	
	struct FPendingGDKInviteData
	{
		FPendingGDKInviteData() = default;
		FPendingGDKInviteData(const int32 ControllerIndex,
							  const FOnlineSessionSearchResult& SearchResult)
		: bControllerIndex(ControllerIndex),
		  bSearchResult(SearchResult)
		{}

		int32 bControllerIndex;
		FOnlineSessionSearchResult bSearchResult;
		bool bHasActiveInvite = false;
	};

	FPendingGDKInviteData PendingGDKInviteData;
	#endif

	TMap<FString, FString> EntityPlatformIdMapping;

	TMap<FString, ECrossNetworkType> VoiceChatPlatforms;
	void GenerateCrossNetworkVoiceChatPlatformPermissions();
	#if defined(OSS_PLAYFAB_PLAYSTATION)
	FString ConnectionString;

	FDelegateHandle OnNativeCreateSessionCompleteDelegateHandle;
	FOnCreateSessionCompleteDelegate OnNativeCreateSessionCompleteDelegate;
	void OnNativeCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	FDelegateHandle OnNativeJoinSessionCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate OnNativeJoinSessionCompleteDelegate;
	void OnNativeJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	FDelegateHandle OnNativeSessionUserInviteAcceptedDelegateHandle;
	FOnSessionUserInviteAcceptedDelegate OnNativeSessionUserInviteAcceptedDelegate;
	void OnNativeSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	#endif // OSS_PLAYFAB_PLAYSTATION
};

typedef TSharedPtr<FOnlineSessionPlayFab, ESPMode::ThreadSafe> FOnlineSessionPlayFabPtr;