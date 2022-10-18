//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "OnlineDelegateMacros.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "Interfaces/OnlineSessionInterface.h"

THIRD_PARTY_INCLUDES_START
#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_PLAYSTATION)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH || OSS_PLAYFAB_PLAYSTATION
#include <PFEntityKey.h>
#include <PFMultiplayer.h>
#include <PFLobby.h>
THIRD_PARTY_INCLUDES_END

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLobbyCreatedAndJoinCompleted, bool, FName);
typedef FOnLobbyCreatedAndJoinCompleted::FDelegate FOnLobbyCreatedAndJoinCompletedDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnJoinLobbyCompleted, FName, EOnJoinSessionCompleteResult::Type JoinSessionResult);
typedef FOnJoinLobbyCompleted::FDelegate FOnJoinLobbyCompletedDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnJoinArrangedLobbyCompleted, FName, bool);
typedef FOnJoinArrangedLobbyCompleted::FDelegate FOnJoinArrangedLobbyCompletedDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnUpdateLobbyCompleted, FName, bool);
typedef FOnUpdateLobbyCompleted::FDelegate FOnUpdateLobbyCompletedDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLobbyUpdate, FName, const PFLobbyUpdatedStateChange&);
typedef FOnLobbyUpdate::FDelegate FOnLobbyUpdateDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLobbyMemberAdded, FName, const PFLobbyMemberAddedStateChange&);
typedef FOnLobbyMemberAdded::FDelegate FOnLobbyMemberAddedDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLobbyMemberRemoved, FName, const PFLobbyMemberRemovedStateChange&);
typedef FOnLobbyMemberRemoved::FDelegate FOnLobbyMemberRemovedDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLeaveLobbyCompleted, FName, bool);
typedef FOnLeaveLobbyCompleted::FDelegate FOnLeaveLobbyCompletedDelegate;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnFindLobbiesCompleted, int32, bool, TSharedPtr<FOnlineSessionSearch>);
typedef FOnFindLobbiesCompleted::FDelegate FOnFindLobbiesCompletedDelegate;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnInvitationReceived, const PFLobbyInviteReceivedStateChange&);
typedef FOnInvitationReceived::FDelegate FOnInvitationReceivedDelegate;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyDisconnected, FName);
typedef FOnLobbyDisconnected::FDelegate FOnLobbyDisconnectedDelegate;

class FPlayFabLobby
{
PACKAGE_SCOPE:
	class FOnlineSubsystemPlayFab* OSSPlayFab = nullptr;
public:
	FPlayFabLobby(FOnlineSubsystemPlayFab* InOSSPlayFab);
	virtual ~FPlayFabLobby() = default;

	bool CreatePlayFabLobby(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings);
	bool CreateLobbyWithUser(TSharedPtr<const FPlayFabUser> InPlayFabUser, FName SessionName, const FOnlineSessionSettings& NewSessionSettings);
	bool JoinLobby(const FUniqueNetId& UserId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession);
	bool JoinLobbyWithUser(TSharedPtr<const FPlayFabUser> InPlayFabUser, FName SessionName, const FOnlineSessionSettings& SessionSettings);
	bool JoinArrangedLobby(FName SessionName, const FOnlineMatchmakingTicketInfoPtr MatchTicket);
	bool UpdateLobby(FName SessionName, const FOnlineSessionSettings& SessionSettings);
	bool AddLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate);
	bool LeaveLobby(const FUniqueNetId& PlayerId, FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate, const FOnUnregisterLocalPlayerCompleteDelegate& UnregisterLocalPlayerCompleteDelegate, bool bDestroyingSession);
	bool FindLobbies(const FUniqueNetId& UserId, TSharedPtr<FOnlineSessionSearch> SearchSettings);
	bool FindFriendLobbies(const FUniqueNetId& UserId);

	bool SendInvite(const FUniqueNetId& SenderId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& FriendUniqueIds);
	void RegisterInvitationListener(const PFEntityKey& ListenerEntity);
	void UnregisterInvitationListener(const PFEntityKey& ListenerEntity);

	void DoWork();

	void HandleCreateAndJoinLobbyCompleted(const PFLobbyCreateAndJoinLobbyCompletedStateChange& StateChange);
	void HandleJoinLobbyCompleted(const PFLobbyJoinLobbyCompletedStateChange& StateChange);
	void HandleJoinArrangedLobbyCompleted(const PFLobbyJoinArrangedLobbyCompletedStateChange& StateChange);
	void HandleLobbyUpdate(const PFLobbyUpdatedStateChange& StateChange);
	void HandleOnMemberAdded(const PFLobbyMemberAddedStateChange& StateChange);
	void HandleOnAddMemberCompleted(const PFLobbyAddMemberCompletedStateChange& StateChange);
	void HandleOnMemberRemoved(const PFLobbyMemberRemovedStateChange& StateChange);
	void HandleLeaveLobbyCompleted(const PFLobbyLeaveLobbyCompletedStateChange& StateChange);
	void HandleFindLobbiesCompleted(const PFLobbyFindLobbiesCompletedStateChange& StateChange);
	void HandleSendInviteCompleted(const PFLobbySendInviteCompletedStateChange& StateChange);
	void HandleInviteListenerStatusChanged(const PFLobbyInviteListenerStatusChangedStateChange& StateChange);
	void HandleInvitationReceived(const PFLobbyInviteReceivedStateChange& StateChange);
	void HandleLobbyDisconnected(const PFLobbyDisconnectedStateChange& StateChange);
	void HandleLobbyDisconnecting(const PFLobbyDisconnectingStateChange& StateChange);
	void HandleForceRemoveMember(const PFLobbyForceRemoveMemberCompletedStateChange& StateChange);

PACKAGE_SCOPE:
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnLobbyCreatedAndJoinCompleted, bool, FName);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnJoinLobbyCompleted, FName, EOnJoinSessionCompleteResult::Type);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnJoinArrangedLobbyCompleted, FName, bool);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnUpdateLobbyCompleted, FName, bool);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnLobbyUpdate, FName, const PFLobbyUpdatedStateChange&);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnLobbyMemberAdded, FName, const PFLobbyMemberAddedStateChange&);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnLobbyMemberRemoved, FName, const PFLobbyMemberRemovedStateChange&);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnLeaveLobbyCompleted, FName, bool);
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnFindLobbiesCompleted,int32, bool, TSharedPtr<FOnlineSessionSearch>);
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnInvitationReceived, const PFLobbyInviteReceivedStateChange&);
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnLobbyDisconnected, FName);

	/** Current search object */
	TSharedPtr<FOnlineSessionSearch> CurrentSessionSearch;
	int32 SearchingUserNum;

private:
	bool GetLobbyFromSession(const FName SessionName, PFLobbyHandle& LobbyHandle);
	bool ValidateSessionForInvite(const FName SessionName);
	FOnlineSessionSearchResult CreateSearchResultFromLobby(const PFLobbySearchResult& Lobby);
	bool IsSearchKey(const FString& Name);
	FString ComposeLobbySearchQueryFilter(const FSearchParams& SearchParams);
	void BuildSearchKeyMappingTable();
	bool GetSearchKeyFromSettingMappingTable(const FString& SettingKey, FString& SearchKey, EOnlineKeyValuePairDataType::Type& Type) const;

	// we can eliminate this map if we pass SessionName as asyncIdentifier to lobby calls
	TMap<PFLobbyHandle, FName> LobbySessionMap;

	struct FPendingSendInviteData
	{
		FName SessionName;
		TSharedPtr<FPlayFabUser> PlayFabSendingUser;
		TArray<FString> FriendUniqueIdStrings;
	};

	struct FAddLocalPlayerData
	{
		FString PlayerPlatformId;
		FOnRegisterLocalPlayerCompleteDelegate RegisterLocalPlayerCompleteDelegate;
	};
	FAddLocalPlayerData AddLocalPlayerData;

	struct FRemoveLocalPlayerData
	{
		FString PlayerPlatformId;
		FOnUnregisterLocalPlayerCompleteDelegate UnregisterLocalPlayerCompleteDelegate;
	};
	FRemoveLocalPlayerData RemoveLocalPlayerData;

	TMap<FString, TPair<FString, EOnlineKeyValuePairDataType::Type>> SearchKeyMappingTable;

public:
	void OnGetPlayFabIDsFromPlatformIDsCompleted(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, FPendingSendInviteData PendingSendInvite);
	void OnGetTitleAccountIDsFromPlatformIDsCompleted(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, FPendingSendInviteData PendingSendInvite);
	void InviteTitleAccountIDsToLobby(FPendingSendInviteData PendingSendInvite, const FString& TitleAccountInviteeId);
};