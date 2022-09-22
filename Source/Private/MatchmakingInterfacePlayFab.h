//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "OnlineDelegateMacros.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "PlayFabLobby.h"

THIRD_PARTY_INCLUDES_START
#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_PLAYSTATION)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH || OSS_PLAYFAB_PLAYSTATION
#include <PFEntityKey.h>
#include <PFMultiplayer.h>
#include <PFMatchmaking.h>
THIRD_PARTY_INCLUDES_END

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMatchmakingTicketCompleted, bool, FName);
typedef FOnMatchmakingTicketCompleted::FDelegate FOnMatchmakingTicketCompletedDelegate;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCancelMatchmakingComplete, FName, bool);
typedef FOnCancelMatchmakingComplete::FDelegate FOnCancelMatchmakingCompleteDelegate;

class FMatchmakingInterfacePlayFab
{
PACKAGE_SCOPE:
	class FOnlineSubsystemPlayFab* OSSPlayFab = nullptr;
public:
	FMatchmakingInterfacePlayFab(FOnlineSubsystemPlayFab* InOSSPlayFab);
	virtual ~FMatchmakingInterfacePlayFab() = default;

	bool CreateMatchMakingTicket(const TArray< TSharedRef<const FUniqueNetId> >& LocalPlayers, 
		FName SessionName, 
		const FOnlineSessionSettings& NewSessionSettings, 
		TSharedRef<FOnlineSessionSearch>& SearchSettings);
	bool CancelMatchmakingTicket(const FName& SessionName);

	void DoWork();

PACKAGE_SCOPE:
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnMatchmakingTicketCompleted, bool, FName);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnCancelMatchmakingComplete, FName, bool);

	/**
	 * Matchmaking related APIs
	 */
	void AddMatchmakingTicket(const FName SessionName, const FOnlineMatchmakingTicketInfoPtr TicketInfo);
	void RemoveMatchmakingTicket(const FName SessionName);
	bool GetMatchmakingTicket(const FName SessionName, FOnlineMatchmakingTicketInfoPtr& OutTicketInfo) const;
	bool GetMatchmakingTicketFromPlayFabTicket(const PFMatchmakingTicketHandle MatchTicket, FName& OutSessionName, FOnlineMatchmakingTicketInfoPtr& OutTicketInfo) const;
	void SetTicketState(const FName SessionName, const EOnlinePlayFabMatchmakingState::Type State);

	void OnJoinArrangedLobbyCompleted(FName SessionName, bool bSucceeded);
	FOnJoinArrangedLobbyCompletedDelegate OnJoinArrangedLobbyCompletedDelegate;
	FDelegateHandle OnJoinArrangedLobbyCompleteDelegateHandle;

private:
	void HandleMatchmakingTicketStatusChanged(const PFMatchmakingTicketStatusChangedStateChange& StateChange);
	void HandleMatchmakingTicketCompleted(const PFMatchmakingTicketCompletedStateChange& StateChange);
	void OnMatchmakingStatusChanged(const FName SessionName, FOnlineMatchmakingTicketInfoPtr Ticket, bool& isRemoved);

	typedef TMap<FName, FOnlineMatchmakingTicketInfoPtr> TicketInfoMap;
	mutable FCriticalSection TicketsLock;
	TicketInfoMap MatchmakingTickets;
};