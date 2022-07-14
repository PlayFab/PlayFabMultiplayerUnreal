//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "MatchmakingInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineIdentityInterfacePlayFab.h"
#include "PlayFabHelpers.h"
#include "PlayFabUtils.h"

#ifdef OSS_PLAYFAB_SWITCH
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH

FMatchmakingInterfacePlayFab::FMatchmakingInterfacePlayFab(FOnlineSubsystemPlayFab* InOSSPlayFab) :
	OSSPlayFab(InOSSPlayFab)
{
	UE_LOG_ONLINE(Verbose, TEXT("FMatchmakingInterfacePlayFab::FMatchmakingInterfacePlayFab: Constructor"));
}

bool FMatchmakingInterfacePlayFab::CreateMatchMakingTicket(const TArray< TSharedRef<const FUniqueNetId> >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG_ONLINE(Verbose, TEXT("CreateMatchMakingTicket"));

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("CreateMatchMakingTicket Identity Interface is invalid"));
		return false;
	}

	FOnlineMatchmakingTicketInfoPtr MatchmakingTicketPtr;
	if (GetMatchmakingTicket(SessionName, MatchmakingTicketPtr))
	{
		UE_LOG_ONLINE(Error, TEXT("CreateMatchMakingTicket MatchmakingTicket already exists, cancel the existing ticket and retry!"));
		return false;
	}

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	TSharedPtr<FPlayFabUser> FirstUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(LocalPlayers[0].Get());
	if (FirstUser == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("CreateMatchMakingTicket GetPartyLocalUserFromPlatformId returned empty user!"));
		return false;
	}

	TArray<PFEntityKey> LocalEntityKeys;
	for (int32 i = 0; i < LocalPlayers.Num(); ++i)
	{
		TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(LocalPlayers[i].Get());
		if (LocalUser == nullptr)
		{
			UE_LOG_ONLINE(Error, TEXT("CreateMatchMakingTicket GetPartyLocalUserFromPlatformId returned empty user for %s!"), *LocalPlayers[i]->ToDebugString());
			return false;
		}
		PFEntityKey EntityKey = LocalUser->GetEntityKey();
		LocalEntityKeys.Add(EntityKey);
	}

	FString QueueName;
	if (!SearchSettings->QuerySettings.Get(SETTING_MATCH_QUEUENAME, QueueName)) 
	{
		QueueName = TEXT("Default");
	}

	std::string QueueNameStr = TCHAR_TO_UTF8(*QueueName);

	// If the timeout is negative we set the max timeout from the service side, which is 1 hour.
	const uint32_t ticketTimeoutInSeconds = (SearchSettings->TimeoutInSeconds < 0) ? 3600 : static_cast<uint32_t>(SearchSettings->TimeoutInSeconds);

	PFMatchmakingTicketConfiguration MatchConfig =
	{
		ticketTimeoutInSeconds,
		QueueNameStr.c_str(),
		0,
		nullptr
	};

	FString UserAttributes;
	if (!SearchSettings->QuerySettings.Get(SETTING_MATCH_ATTRIBUTES_KEY, UserAttributes))
	{
		// PlayFab OSS expects the UserAttributes to be part of SearchSettings, however the GDK OSS reads the value from FOnlineSessionSetting.
		// hence read the values from FOnlineSessionSetting if we don't see any attributes populated in SearchSettings
		const FOnlineSessionSetting* AttributesSetting = NewSessionSettings.Settings.Find(SETTING_MATCH_ATTRIBUTES_KEY);
		if (AttributesSetting)
		{
			AttributesSetting->Data.GetValue(UserAttributes);
		}
		else
		{
			UserAttributes = TEXT("");
		}
	}

	// Now convert the json to char** string to pass onto CreateMatchmakingTicket API
	AnsiStringList UserAttributesStr;
	for (auto Iterator : LocalEntityKeys)
	{
		UserAttributesStr.Add(UserAttributes);
	}

	PFMatchmakingTicketHandle MatchTicket;
	HRESULT Hr = PFMultiplayerCreateMatchmakingTicket(
		OSSPlayFab->GetMultiplayerHandle(),
		LocalEntityKeys.Num(),
		LocalEntityKeys.GetData(),
		UserAttributesStr.GetData(),
		&MatchConfig,
		nullptr,
		&MatchTicket);

	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FMatchmakingInterfacePlayFab::PFMultiplayerCreateMatchmakingTicket failed: Error code [0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		return false;
	}

	MatchmakingTicketPtr = MakeShared<FOnlineMatchmakingTicketInfo>();
	AddMatchmakingTicket(SessionName, MatchmakingTicketPtr);

	MatchmakingTicketPtr->MatchmakingState = EOnlinePlayFabMatchmakingState::CreatingTicket;
	MatchmakingTicketPtr->SessionName = SessionName;
	MatchmakingTicketPtr->QueueName = QueueName;
	MatchmakingTicketPtr->SessionSearch = SearchSettings;
	MatchmakingTicketPtr->SessionSettings = NewSessionSettings;
	MatchmakingTicketPtr->PlayFabMatchTicket = MatchTicket;
	MatchmakingTicketPtr->SetHostUser(FirstUser);
	MatchmakingTicketPtr->SearchingPlayerNetId = FUniqueNetIdPlayFab::Create(LocalPlayers[0].Get());

	return true;
}

bool FMatchmakingInterfacePlayFab::CancelMatchmakingTicket(const FName& SessionName)
{
	FOnlineMatchmakingTicketInfoPtr MatchmakingTicketPtr;
	if (!GetMatchmakingTicket(SessionName, MatchmakingTicketPtr) || !MatchmakingTicketPtr.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FMatchmakingInterfacePlayFab::CancelMatchmakingTicket failed: Cannot find ticket for session: %s"), *SessionName.ToString());
		TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
		return false;
	}

	HRESULT Hr = PFMatchmakingTicketCancel(MatchmakingTicketPtr->PlayFabMatchTicket);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FMatchmakingInterfacePlayFab::PFMatchmakingTicketCancel failed. ErrorCode=[0x%08x]"), Hr);
		TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
		return false;
	}

	return true;
}

void FMatchmakingInterfacePlayFab::DoWork()
{
	uint32 StateChangeCount = 0u;
	const PFMatchmakingStateChange* const* StateChanges = nullptr;

	HRESULT Hr = PFMultiplayerStartProcessingMatchmakingStateChanges(OSSPlayFab->GetMultiplayerHandle(), &StateChangeCount, &StateChanges);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FMatchmakingInterfacePlayFab::DoWork::StartProcessingStateChanges failed. ErrorCode=[0x%08x], Error message %s"), Hr, *GetMultiplayerErrorMessage(Hr));
		return;
	}

	for (uint32 i = 0; i < StateChangeCount; ++i)
	{
		const PFMatchmakingStateChange& StateChange = *StateChanges[i];

		switch (StateChange.stateChangeType)
		{
			case PFMatchmakingStateChangeType::TicketStatusChanged:
			{
				HandleMatchmakingTicketStatusChanged(static_cast<const PFMatchmakingTicketStatusChangedStateChange&>(StateChange));
				break;
			}
			case PFMatchmakingStateChangeType::TicketCompleted:
			{
				HandleMatchmakingTicketCompleted(static_cast<const PFMatchmakingTicketCompletedStateChange&>(StateChange));
				break;
			}
			default:
			{
				UE_LOG_ONLINE(Verbose, TEXT("Received PlayFab Unhandled(%u) event in FMatchmakingInterfacePlayFab"), StateChange.stateChangeType);
				break;
			}
		}
	}
	Hr = PFMultiplayerFinishProcessingMatchmakingStateChanges(OSSPlayFab->GetMultiplayerHandle(), StateChangeCount, StateChanges);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FMatchmakingInterfacePlayFab::DoWork::PFMultiplayerFinishProcessingMatchmakingStateChanges failed. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
	}
}

void FMatchmakingInterfacePlayFab::HandleMatchmakingTicketStatusChanged(const PFMatchmakingTicketStatusChangedStateChange& StateChange)
{
	PFMatchmakingTicketStatus Status;
	HRESULT Hr = PFMatchmakingTicketGetStatus(StateChange.ticket, &Status);
	UE_LOG_ONLINE(Verbose, TEXT("FMatchmakingInterfacePlayFab::HandleMatchmakingTicketStatusChanged(%u) event: for ticket (%p), Status: (%u). ErrorCode=[0x%08x], Error message:%s"), StateChange.stateChangeType, StateChange.ticket, Status, Hr, *GetMultiplayerErrorMessage(Hr));

	FName SessionName;
	FOnlineMatchmakingTicketInfoPtr Ticket;
	if (GetMatchmakingTicketFromPlayFabTicket(StateChange.ticket, SessionName, Ticket))
	{
		bool isRemoved = false;
		OnMatchmakingStatusChanged(SessionName, Ticket, isRemoved);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("Unknown Matchmaking ticket!"));
	}
}

void FMatchmakingInterfacePlayFab::HandleMatchmakingTicketCompleted(const PFMatchmakingTicketCompletedStateChange& StateChange)
{
	PFMatchmakingTicketStatus Status;
	HRESULT Hr = PFMatchmakingTicketGetStatus(StateChange.ticket, &Status);
	UE_LOG_ONLINE(Verbose, TEXT("FMatchmakingInterfacePlayFab::HandleMatchmakingTicketCompleted(%u) event: for ticket (%p), Status: (%u). ErrorCode=[0x%08x], Error message:%s"), StateChange.stateChangeType, StateChange.ticket, Status, Hr, *GetMultiplayerErrorMessage(Hr));

	FOnlineMatchmakingTicketInfoPtr Ticket;
	FName SessionName;
	PFMatchmakingTicketHandle PlayFabMatchTicket = StateChange.ticket;

	if (!GetMatchmakingTicketFromPlayFabTicket(PlayFabMatchTicket, SessionName, Ticket))
	{
		UE_LOG_ONLINE(Error, TEXT("Unknown Matchmaking ticket!"));
		return;
	}

	if (SUCCEEDED(StateChange.result))
	{
		if (Status == PFMatchmakingTicketStatus::Matched)
		{
			const PFMatchmakingMatchDetails* MatchDetails;
			Hr = PFMatchmakingTicketGetMatch(PlayFabMatchTicket, &MatchDetails);
			if (FAILED(Hr))
			{
				UE_LOG_ONLINE(Error, TEXT("FMatchmakingInterfacePlayFab::HandleMatchmakingTicketCompleted PFMatchmakingTicketGetMatch failed. ErrorCode=[0x%08x], Error message %s"), Hr, *GetMultiplayerErrorMessage(Hr));
				Ticket->MatchmakingState = EOnlinePlayFabMatchmakingState::Failed;
			}
			else
			{
				Ticket->PlayFabMatchmakingDetails = MatchDetails;
				Ticket->MatchmakingState = EOnlinePlayFabMatchmakingState::MatchFound;
			}
		}
		else if (Status == PFMatchmakingTicketStatus::Canceled)
		{
			UE_LOG_ONLINE(Error, TEXT("MatchmakingTicketResult::Canceled, Error code:[0x%08x],Error message %s"), StateChange.result, *GetMultiplayerErrorMessage(StateChange.result));
			Ticket->MatchmakingState = EOnlinePlayFabMatchmakingState::Cancelled;
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("MatchmakingTicketResult::Failed, Error code:[0x%08x],Error message %s"), StateChange.result, *GetMultiplayerErrorMessage(StateChange.result));
			Ticket->MatchmakingState = EOnlinePlayFabMatchmakingState::Failed;
		}
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("PFMatchmakingTicketCompletedStateChange Failed, Error code:[0x%08x],Error message %s"), StateChange.result, *GetMultiplayerErrorMessage(StateChange.result));
		if (Status == PFMatchmakingTicketStatus::Canceled)
		{
			Ticket->MatchmakingState = EOnlinePlayFabMatchmakingState::Cancelled;
		}
		else
		{
			Ticket->MatchmakingState = EOnlinePlayFabMatchmakingState::Failed;
		}
	}

	bool isTicketRemoved;
	OnMatchmakingStatusChanged(SessionName, Ticket, isTicketRemoved);
}

void FMatchmakingInterfacePlayFab::OnMatchmakingStatusChanged(const FName SessionName, FOnlineMatchmakingTicketInfoPtr Ticket, bool& isRemoved)
{
	if (!Ticket.IsValid())
	{
		return;
	}

	switch (Ticket->MatchmakingState)
	{
		case EOnlinePlayFabMatchmakingState::TicketCreated:
		{
			break;
		}
		case EOnlinePlayFabMatchmakingState::MatchFound:
		{
			auto SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
			FNamedOnlineSessionPtr ExistingNamedSession = SessionInterface->GetNamedSessionPtr(SessionName);
			if (!ExistingNamedSession.IsValid())
			{
				// Create a named session from the search result data
				FNamedOnlineSessionRef NamedSession = SessionInterface->AddNamedSessionRef(SessionName, Ticket->SessionSettings);
				NamedSession->HostingPlayerNum = INDEX_NONE;
				NamedSession->OwningUserId = Ticket->SearchingPlayerNetId;
				NamedSession->LocalOwnerId = Ticket->SearchingPlayerNetId;

				OnJoinArrangedLobbyCompletedDelegate = FOnJoinArrangedLobbyCompletedDelegate::CreateRaw(this, &FMatchmakingInterfacePlayFab::OnJoinArrangedLobbyCompleted);
				OnJoinArrangedLobbyCompleteDelegateHandle = OSSPlayFab->GetPlayFabLobbyInterface()->AddOnJoinArrangedLobbyCompletedDelegate_Handle(OnJoinArrangedLobbyCompletedDelegate);

				if (!OSSPlayFab->GetPlayFabLobbyInterface()->JoinArrangedLobby(SessionName, Ticket))
				{
					UE_LOG_ONLINE_SESSION(Log, TEXT("Failed JoinArrangedLobby operation"));
					RemoveMatchmakingTicket(SessionName);
					TriggerOnMatchmakingTicketCompletedDelegates(false, SessionName);
					isRemoved = true;
					return;
				}
				Ticket->MatchmakingState = EOnlinePlayFabMatchmakingState::JoiningArrangedLobby;
			}
			break;
		}
		case EOnlinePlayFabMatchmakingState::Cancelled:
		{
			UE_LOG_ONLINE(Verbose, TEXT("FMatchmakingInterfacePlayFab::OnMatchmakingStatusChanged Cancelled"));
			TriggerOnCancelMatchmakingCompleteDelegates(SessionName, true);
			RemoveMatchmakingTicket(SessionName);
			isRemoved = true;
			return;
		}
		case EOnlinePlayFabMatchmakingState::Failed:
		{
			UE_LOG_ONLINE(Verbose, TEXT("FMatchmakingInterfacePlayFab::OnMatchmakingStatusChanged Failed"));
			TriggerOnMatchmakingTicketCompletedDelegates(false, SessionName);

			PFMatchmakingTicketStatus Status;
			HRESULT Hr = PFMatchmakingTicketGetStatus(Ticket->PlayFabMatchTicket, &Status);
			if (Status == PFMatchmakingTicketStatus::Failed)
			{
				RemoveMatchmakingTicket(SessionName);
				isRemoved = true;
			}

			return;
		}
		case EOnlinePlayFabMatchmakingState::JoiningArrangedLobby:
		{
			break;
		}
		case EOnlinePlayFabMatchmakingState::JoinArrangedLobbyCompleted:
		{
			UE_LOG_ONLINE(Verbose, TEXT("FMatchmakingInterfacePlayFab JoinArrangedLobbyCompleted"));
			RemoveMatchmakingTicket(SessionName);
			isRemoved = true;
			return;
		}
	}
}

void FMatchmakingInterfacePlayFab::OnJoinArrangedLobbyCompleted(FName SessionName, bool bSucceeded)
{
	OSSPlayFab->GetPlayFabLobbyInterface()->ClearOnJoinArrangedLobbyCompletedDelegate_Handle(OnJoinArrangedLobbyCompleteDelegateHandle);

	FOnlineMatchmakingTicketInfoPtr Ticket;
	GetMatchmakingTicket(SessionName, Ticket);
	if (!Ticket.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("OnJoinArrangedLobbyCompleted invalid ticket!"));
		RemoveMatchmakingTicket(SessionName);
		TriggerOnMatchmakingTicketCompletedDelegates(false, SessionName);
		return;
	}
	Ticket->MatchmakingState = EOnlinePlayFabMatchmakingState::JoinArrangedLobbyCompleted;
	TriggerOnMatchmakingTicketCompletedDelegates(bSucceeded, SessionName);

	bool isTicketRemoved;
	OnMatchmakingStatusChanged(SessionName, Ticket, isTicketRemoved);
}

void FMatchmakingInterfacePlayFab::AddMatchmakingTicket(const FName SessionName, const FOnlineMatchmakingTicketInfoPtr InTicketInfo)
{
	FScopeLock ScopeLock(&TicketsLock);
	MatchmakingTickets.Add(SessionName, InTicketInfo);
}

void FMatchmakingInterfacePlayFab::RemoveMatchmakingTicket(const FName SessionName)
{
	FScopeLock ScopeLock(&TicketsLock);

	FOnlineMatchmakingTicketInfoPtr MatchmakingTicketPtr;
	if (!GetMatchmakingTicket(SessionName, MatchmakingTicketPtr) || !MatchmakingTicketPtr.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FMatchmakingInterfacePlayFab::RemoveMatchmakingTicket failed: Cannot find ticket for session")); // TODO: It doesn't like to accept SessionName or *SessionName into a %s parameter here
		return;
	}

	MatchmakingTickets.Remove(SessionName);
	HRESULT Hr = PFMultiplayerDestroyMatchmakingTicket(OSSPlayFab->GetMultiplayerHandle(), MatchmakingTicketPtr->PlayFabMatchTicket);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FMatchmakingInterfacePlayFab::PFMultiplayerDestroyMatchmakingTicket failed. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
	}
}

bool FMatchmakingInterfacePlayFab::GetMatchmakingTicket(const FName SessionName, FOnlineMatchmakingTicketInfoPtr& OutTicketInfo) const
{
	FScopeLock ScopeLock(&TicketsLock);

	const FOnlineMatchmakingTicketInfoPtr* FoundTicket = MatchmakingTickets.Find(SessionName);

	if (FoundTicket)
	{
		OutTicketInfo = *FoundTicket;
	}

	return (FoundTicket != nullptr);
}

bool FMatchmakingInterfacePlayFab::GetMatchmakingTicketFromPlayFabTicket(const PFMatchmakingTicketHandle MatchTicket, FName& OutSessionName, FOnlineMatchmakingTicketInfoPtr& OutTicketInfo) const
{
	FScopeLock ScopeLock(&TicketsLock);
	for (auto Iterator : MatchmakingTickets)
	{
		FName SessionName = Iterator.Key;
		FOnlineMatchmakingTicketInfoPtr Ticket = Iterator.Value;
		if (!Ticket.IsValid())
		{
			continue;
		}
		if (Ticket->PlayFabMatchTicket == MatchTicket)
		{
			OutSessionName = SessionName;
			OutTicketInfo = Ticket;
			return true;
		}
	}

	return false;
}

void FMatchmakingInterfacePlayFab::SetTicketState(const FName SessionName, const EOnlinePlayFabMatchmakingState::Type State)
{
	FOnlineMatchmakingTicketInfoPtr MatchmakingTicketPtr;
	GetMatchmakingTicket(SessionName, MatchmakingTicketPtr);

	if (MatchmakingTicketPtr.IsValid())
	{
		MatchmakingTicketPtr->MatchmakingState = State;
	}
}