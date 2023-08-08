//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#if defined(OSS_PLAYFAB_GDK)
#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineIdentityInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineSubsystemGDK.h"
#include "GenericPlatform/GenericPlatformHttp.h"

THIRD_PARTY_INCLUDES_START
#include <xsapi-c/multiplayer_activity_c.h>
#include <XGameInvite.h>
#include <XGameRuntimeFeature.h>
THIRD_PARTY_INCLUDES_END

static FString FindUrlParameter(FString Url, const TCHAR* Name)
{
	FString StartToken = FString::Printf(TEXT("%s"), Name);
	int32 Start = Url.Find(StartToken);
	if (Start == INDEX_NONE)
	{
		return FString();
	}

	Start += StartToken.Len();
	int32 End = Url.Find(TEXT("&"), ESearchCase::IgnoreCase, ESearchDir::FromStart, Start);

	// If the session is at the end of the string then end will return not found.
	int32 CharCount = MAX_int32;
	if (End != INDEX_NONE)
	{
		CharCount = End - Start;
	}

	return(Url.Mid(Start, CharCount));
}

uint64 GetXuidForEntityKey(FOnlineSubsystemPlayFab* OSSPlayFab, const PFEntityKey& EntityKey)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::GetXuidForEntityKey()"));

	uint64 Xuid = 0;

	FOnlineIdentityPlayFabPtr PlayFabIdentityInt = OSSPlayFab ? OSSPlayFab->GetIdentityInterfacePlayFab() : nullptr;
	if (PlayFabIdentityInt.IsValid())
	{
		FString EntityId = FString(EntityKey.id);
		TSharedPtr<FPlayFabUser> PartyLocalUser = PlayFabIdentityInt->GetPartyLocalUserFromEntityIdString(EntityId);
		if (PartyLocalUser.IsValid())
		{
			FString XuidStr = PartyLocalUser->GetPlatformUserId();
			Xuid = FCString::Strtoi64(*XuidStr, NULL, 10);
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::GetXuidForEntityKey: PartyLocalUser was null"));
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::GetXuidForEntityKey: PlayFabIdentityInt was null"));
	}

	return Xuid;
}

FGDKContextHandle GetGDKContextForXuid(uint64 Xuid)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::GetGDKContextForXuid()"));

	if (FOnlineSubsystemGDK* GDKSubsystem = static_cast<FOnlineSubsystemGDK*>(IOnlineSubsystem::Get(GDK_SUBSYSTEM)))
	{
		return GDKSubsystem->GetGDKContext(Xuid);
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::GetGDKContextForXuid: GDKSubsystem was null"));
	}

	return FGDKContextHandle();
}

void FOnlineSessionPlayFab::RegisterForInvites()
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RegisterForInvites()"));

	if (XGameRuntimeIsFeatureAvailable(XGameRuntimeFeature::XGameInvite))
	{
		auto InviteHandlerLambda = [](void* Context, const char* InviteUri)
		{
			reinterpret_cast<FOnlineSessionPlayFab*>(Context)->SaveInviteFromEvent(Context, FString(UTF8_TO_TCHAR(InviteUri)));
		};

		HRESULT Hr = XGameInviteRegisterForEvent(FGDKAsyncTaskQueue::GetGenericQueue(), this, InviteHandlerLambda, &InviteAcceptedHandler);
		if (FAILED(Hr))
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::RegisterForUpdates: XGameInviteRegisterForEvent failed: ErrorCode=[0x%08x]!"), Hr);
		}
	}
}

void FOnlineSessionPlayFab::UnregisterForInvites()
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::UnregisterForInvites()"));

	XGameInviteUnregisterForEvent(InviteAcceptedHandler, true /* Wait for pending event callbacks to complete.*/);
}

bool FOnlineSessionPlayFab::SendInvite(const FUniqueNetId& SenderId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& RemoteUserNetIds)
{
	if (!SenderId.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot Invite friend to session %s, SenderId is invalid"), *SessionName.ToString());
		return false;
	}

	if (RemoteUserNetIds.Num() < 1)
	{
		// Return true in this case, but log it since it's strange
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Attempted to invite any empty array of friends to session %s"), *SessionName.ToString());
		return true;
	}

	for (const TSharedRef<const FUniqueNetId>& Friend : RemoteUserNetIds)
	{
		if (!Friend->IsValid())
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot Invite invalid friend to session %s"), *SessionName.ToString());
			return false;
		}
	}

	if (!ValidateSessionForInvite(SessionName))
	{
		return false;
	}

	PFLobbyHandle LobbyHandle{};
	if (!GetLobbyHandleBySessionName(SessionName, LobbyHandle))
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SendInvite: No lobby found for session %s!"), *(SessionName.ToString()));
		return false;
	}

	const char* ConnectionStr;
	HRESULT Hr = PFLobbyGetConnectionString(LobbyHandle, &ConnectionStr);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFLobbyGetConnectionString", Hr);
		return false;
	}

	uint64 SenderXuid = FCString::Strtoi64(*SenderId.ToString(), NULL, 10);
	FGDKContextHandle GDKContext = GetGDKContextForXuid(SenderXuid);
	if (!GDKContext.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Error, TEXT("FOnlineSessionPlayFab::SendInvite: GDKContext was null"));
		return false;
	}

	TUniquePtr<XAsyncBlock> NewAsyncBlock = MakeUnique<XAsyncBlock>();
	NewAsyncBlock->queue = FGDKAsyncTaskQueue::GetGenericQueue();
	NewAsyncBlock->callback = [](XAsyncBlock* ab)
	{
		TUniquePtr<XAsyncBlock> AsyncBlock{ ab }; // take ownership of XAsyncBlock
		HRESULT Hr = XAsyncGetStatus(ab, false);
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SendInvite: XAsyncGetStatus: result 0x%08x"), Hr);
	};

	TArray<uint64> Xuids;
	for (const TSharedRef<const FUniqueNetId>& RemoteUserNetId : RemoteUserNetIds)
	{
		uint64 Xuid = FCString::Strtoi64(*RemoteUserNetId->ToString(), NULL, 10);
		Xuids.Add(Xuid);
	}

	Hr = XblMultiplayerActivitySendInvitesAsync(GDKContext,
		Xuids.GetData(),
		Xuids.Num(),
		true /*Send the invite to platforms that may be different than senders platform*/,
		ConnectionStr,
		NewAsyncBlock.Get());

	if (SUCCEEDED(Hr))
	{
		NewAsyncBlock.Release();
		return true;
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Error, TEXT("FOnlineSessionPlayFab::SendInvite: XblMultiplayerActivitySendInvitesAsync: failed with ErrorCode=[0x%08x]."), Hr);
		return false;
	}
}

void FOnlineSessionPlayFab::SaveInviteFromEvent(void* Context, const FString& ActivationUri)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SaveInviteFromEvent: InviteUri: %s"), *ActivationUri);

	FString ConnectionStringKeyWord = TEXT("connectionString=");
	FString EncodedConnectionString = FindUrlParameter(ActivationUri, *ConnectionStringKeyWord);
	if (EncodedConnectionString.IsEmpty())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SaveInviteFromEvent: EncodedConnectionString was invalid"));
		return;
	}

	// connectionString is encoded in the URI - use Engine API to decode it
	FString ConnectionStr = FGenericPlatformHttp::UrlDecode(EncodedConnectionString);

	FOnlineSessionSearchResult SearchResult;
	SearchResult.Session.SessionSettings.Set(SETTING_CONNECTION_STRING, FString(ConnectionStr), EOnlineDataAdvertisementType::ViaOnlineService);
	
	TSharedRef<FOnlineSessionInfoPlayFab> NewSessionInfo = MakeShared<FOnlineSessionInfoPlayFab>();
	SearchResult.Session.SessionInfo = NewSessionInfo;
	NewSessionInfo->ConnectionString = ConnectionStr;

	const int32 ControllerIndex = 0;
	OSSPlayFab->ExecuteNextTick([this, ControllerIndex, SearchResult]()
	{
		PendingInviteData = FPendingInviteData(ControllerIndex, SearchResult);
		PendingInviteData.bHasActiveInvite = true;
	});
}

void FOnlineSessionPlayFab::TickPendingInvites()
{
	if (PendingInviteData.bHasActiveInvite)
	{
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::TickPendingInvites: Processing pending invite"));

		FOnlineIdentityPlayFabPtr PlayFabIdentityInt = OSSPlayFab ? OSSPlayFab->GetIdentityInterfacePlayFab() : nullptr;
		if (PlayFabIdentityInt.IsValid())
		{
			if (PlayFabIdentityInt && PlayFabIdentityInt->GetAllPartyLocalUsers().Num() > 0 && OnSessionUserInviteAcceptedDelegates.IsBound())
			{
				TSharedPtr<const FUniqueNetId> LocalUniqueId = PlayFabIdentityInt->GetUniquePlayerId(PendingInviteData.bControllerIndex);
				TriggerOnSessionUserInviteAcceptedDelegates(true, PendingInviteData.bControllerIndex, LocalUniqueId, PendingInviteData.bSearchResult);
				PendingInviteData = FPendingInviteData(); // reset pending invite
			}
		}
	}
}

void FOnlineSessionPlayFab::SetMultiplayerActivityForSession(const FNamedOnlineSessionPtr& ExistingNamedSession)
{
	if (ExistingNamedSession.IsValid() == false)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivityForSession: ExistingNamedSession was null"));
		return;
	}

	if (ExistingNamedSession->SessionSettings.bUsesPresence == false)
	{
		//don't set an activity based on this session
		return;
	}

	FOnlineIdentityPlayFabPtr PlayFabIdentityInt = OSSPlayFab ? OSSPlayFab->GetIdentityInterfacePlayFab() : nullptr;
	if (PlayFabIdentityInt.IsValid() == false)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivityForSession: Identity Interface is invalid"));
		return;
	}

	FOnlineSessionInfoPlayFabPtr SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(ExistingNamedSession->SessionInfo);
	if (SessionInfo.IsValid() == false)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivityForSession: SessionInfo was null"));
		return;
	}

	SetMultiplayerActivity(SessionInfo->LobbyHandle, PlayFabIdentityInt->GetLocalUserEntityKeys(), ExistingNamedSession->SessionSettings);
}

void FOnlineSessionPlayFab::SetMultiplayerActivity(PFLobbyHandle LobbyHandle, const PFEntityKey& EntityKey, const FOnlineSessionSettings& OnlineSessionSettings) const
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivity()"));

	TArray<PFEntityKey> EntityKeys;
	EntityKeys.Add(EntityKey);

	SetMultiplayerActivity(LobbyHandle, EntityKeys, OnlineSessionSettings);
}

void FOnlineSessionPlayFab::SetMultiplayerActivity(PFLobbyHandle LobbyHandle, const TArray<PFEntityKey>& EntityKeys, const FOnlineSessionSettings& OnlineSessionSettings) const
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivity()"));

	if (OnlineSessionSettings.bUsesPresence == false)
	{
		//don't set an activity based on this session
		return;
	}

	const char* connectionString;
	HRESULT Hr = PFLobbyGetConnectionString(LobbyHandle, &connectionString);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFLobbyGetConnectionString", Hr);
		return;
	}

	uint32 MaxPlayerCount;
	Hr = PFLobbyGetMaxMemberCount(LobbyHandle, &MaxPlayerCount);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFLobbyGetMaxMemberCount", Hr);
		return;
	}

	uint32 LobbyMemberCount;
	const PFEntityKey* LobbyMembers;
	Hr = PFLobbyGetMembers(LobbyHandle, &LobbyMemberCount, &LobbyMembers);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFLobbyGetMembers", Hr);
		return;
	}

	const char* LobbyId;
	Hr = PFLobbyGetLobbyId(LobbyHandle, &LobbyId);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFLobbyGetLobbyId", Hr);
		return;
	}

	XblMultiplayerActivityJoinRestriction JoinRestriction = XblMultiplayerActivityJoinRestriction::InviteOnly;
	if (OnlineSessionSettings.bShouldAdvertise)
	{
		JoinRestriction = XblMultiplayerActivityJoinRestriction::Public;
	}
	else if (OnlineSessionSettings.bAllowJoinViaPresence)
	{
		JoinRestriction = XblMultiplayerActivityJoinRestriction::Followed;
	}

	for (const PFEntityKey& EntityKey : EntityKeys)
	{
		uint64 Xuid = GetXuidForEntityKey(OSSPlayFab, EntityKey);
		FGDKContextHandle GDKContext = GetGDKContextForXuid(Xuid);
		if (GDKContext.IsValid())
		{
			TUniquePtr<XAsyncBlock> NewAsyncBlock = MakeUnique<XAsyncBlock>();
			NewAsyncBlock->queue = FGDKAsyncTaskQueue::GetGenericQueue();

			NewAsyncBlock->callback = [](XAsyncBlock* ab)
			{
				TUniquePtr<XAsyncBlock> AsyncBlock{ ab }; // take ownership of XAsyncBlock
				HRESULT Hr = XAsyncGetStatus(ab, false);
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivities: XAsyncGetStatus: result 0x%08x"), Hr);
			};

			XblMultiplayerActivityInfo MultiplayerActivityInfo{};
			MultiplayerActivityInfo.connectionString = connectionString;
			MultiplayerActivityInfo.joinRestriction = JoinRestriction;
			MultiplayerActivityInfo.maxPlayers = MaxPlayerCount;
			MultiplayerActivityInfo.currentPlayers = LobbyMemberCount;
			MultiplayerActivityInfo.groupId = LobbyId;
			MultiplayerActivityInfo.xuid = Xuid;

			Hr = XblMultiplayerActivitySetActivityAsync(GDKContext, &MultiplayerActivityInfo, true, NewAsyncBlock.Get());
			if (SUCCEEDED(Hr))
			{
				NewAsyncBlock.Release();
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivities: XblMultiplayerActivitySetActivityAsync: failed with ErrorCode=[0x%08x]"), Hr);
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::SetMultiplayerActivities: GDKContext was null"));
		}
	}
}

void FOnlineSessionPlayFab::DeleteMultiplayerActivity(PFLobbyHandle LobbyHandle, const PFEntityKey& EntityKey, const FOnlineSessionSettings& OnlineSessionSettings) const
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::DeleteMultiplayerActivity()"));

	TArray<PFEntityKey> EntityKeys;
	EntityKeys.Add(EntityKey);

	DeleteMultiplayerActivity(LobbyHandle, EntityKeys, OnlineSessionSettings);
}

void FOnlineSessionPlayFab::DeleteMultiplayerActivity(PFLobbyHandle LobbyHandle, const TArray<PFEntityKey>& EntityKeys, const FOnlineSessionSettings& OnlineSessionSettings) const
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::DeleteMultiplayerActivities()"));

	if (OnlineSessionSettings.bUsesPresence == false)
	{
		//this session shouldn't have an activity to delete
		return;
	}

	for (const PFEntityKey& EntityKey : EntityKeys)
	{
		uint64 Xuid = GetXuidForEntityKey(OSSPlayFab, EntityKey);
		FGDKContextHandle GDKContext = GetGDKContextForXuid(Xuid);
		if (GDKContext.IsValid())
		{
			TUniquePtr<XAsyncBlock> NewAsyncBlock = MakeUnique<XAsyncBlock>();
			NewAsyncBlock->queue = FGDKAsyncTaskQueue::GetGenericQueue();
			NewAsyncBlock->callback = [](XAsyncBlock* ab)
			{
				TUniquePtr<XAsyncBlock> AsyncBlock{ ab }; // take ownership of XAsyncBlock
				HRESULT Hr = XAsyncGetStatus(ab, false);
				UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::DeleteMultiplayerActivities: XAsyncGetStatus: result 0x%08x"), Hr);
			};

			HRESULT Hr = XblMultiplayerActivityDeleteActivityAsync(GDKContext, NewAsyncBlock.Get());
			if (SUCCEEDED(Hr))
			{
				NewAsyncBlock.Release();
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::DeleteMultiplayerActivities: XblMultiplayerActivityDeleteActivityAsync: failed with ErrorCode=[0x%08x]"), Hr);
			}
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::DeleteMultiplayerActivities: GDKContext was null"));
		}
	}
}

void FOnlineSessionPlayFab::RecordRecentlyMetPlayer(PFLobbyHandle LobbyHandle, const PFEntityKey& EntityKey, const PFEntityKey& RecentPlayerEntityKey, const FString& RecentPlayerPlatformIdStr) const
{
	TArray<PFEntityKey> EntityKeys;
	EntityKeys.Add(EntityKey);

	RecordRecentlyMetPlayer(LobbyHandle, EntityKeys, RecentPlayerEntityKey, RecentPlayerPlatformIdStr);
}

void FOnlineSessionPlayFab::RecordRecentlyMetPlayer(PFLobbyHandle LobbyHandle, const TArray<PFEntityKey>& EntityKeys, const PFEntityKey& RecentPlayerEntityKey, const FString& RecentPlayerPlatformIdStr) const
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::RecordRecentlyMetPlayer()"));

	FOnlineIdentityPlayFabPtr PlayFabIdentityInt = OSSPlayFab ? OSSPlayFab->GetIdentityInterfacePlayFab() : nullptr;
	if (!PlayFabIdentityInt.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::RecordRecentlyMetPlayer: PlayFabIdentityInt was null"));
		return;
	}

	if (PlayFabIdentityInt->IsUserLocal(RecentPlayerEntityKey))
	{
		return;
	}

	FString RecentPlayerXuidStr(RecentPlayerPlatformIdStr);
	uint64 RecentPlayerXuid = FCString::Strtoi64(*RecentPlayerXuidStr, NULL, 10);
	XblMultiplayerActivityRecentPlayerUpdate RecentPlayer{ RecentPlayerXuid };

	for (const PFEntityKey& EntityKey : EntityKeys)
	{
		uint64 Xuid = GetXuidForEntityKey(OSSPlayFab, EntityKey);
		FGDKContextHandle GDKContext = GetGDKContextForXuid(Xuid);
		if (GDKContext.IsValid())
		{
			HRESULT Hr = XblMultiplayerActivityUpdateRecentPlayers(GDKContext, &RecentPlayer, 1);
			if (FAILED(Hr))
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::RecordRecentlyMetPlayer: XblMultiplayerActivityUpdateRecentPlayers: failed with ErrorCode=[0x%08x]"), Hr);
			}
		}
	}
}
#endif // OSS_PLAYFAB_GDK