//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "PlayFabLobby.h"
#include "PlayFabUtils.h"
#include "PlayFabHelpers.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineSessionInterfacePlayFab.h"
#include "Online/OnlineSessionNames.h"

static struct FSearchKeyMappingTable
{
	FName SettingKey;
	int32 KeyNumber;
	EOnlineKeyValuePairDataType::Type Type;
} s_SearchKeyMappingTable[] =
{
	// String
	{SETTING_MAPNAME, 					30, EOnlineKeyValuePairDataType::String},
	{SETTING_GAMEMODE, 					29, EOnlineKeyValuePairDataType::String},
	{SETTING_REGION, 					28, EOnlineKeyValuePairDataType::String},
	{SETTING_SUBREGION, 				27, EOnlineKeyValuePairDataType::String},
	{SETTING_DCID, 						26, EOnlineKeyValuePairDataType::String},
	{SETTING_ALLOWBROADCASTING, 		25, EOnlineKeyValuePairDataType::String},
	{SETTING_SESSIONKEY, 				24, EOnlineKeyValuePairDataType::String},
	{SETTING_CUSTOM, 					23, EOnlineKeyValuePairDataType::String},
	{SETTING_MATCHING_HOPPER, 			22, EOnlineKeyValuePairDataType::String},
	{SETTING_MATCHING_ATTRIBUTES, 		21, EOnlineKeyValuePairDataType::String},
	{SETTING_MATCH_MEMBERS_JSON, 		20, EOnlineKeyValuePairDataType::String},
	{SETTING_SESSION_MEMBER_CONSTANT_CUSTOM_JSON_XUID_PREFIX, 19, EOnlineKeyValuePairDataType::String},
	{SETTING_SESSION_TEMPLATE_NAME, 	18, EOnlineKeyValuePairDataType::String},
	{SETTING_GAME_SESSION_URI, 			17, EOnlineKeyValuePairDataType::String},
	{SETTING_GROUP_NAME_PREFIX, 		16, EOnlineKeyValuePairDataType::String},
	{SETTING_CUSTOM_JOIN_INFO, 			15, EOnlineKeyValuePairDataType::String},
	{SETTING_SESSION_SUBSCRIPTION_TYPES, 14, EOnlineKeyValuePairDataType::String},
	{SETTING_HOST_MIGRATION, 			13, EOnlineKeyValuePairDataType::String},
	{SEARCH_DEDICATED_ONLY, 			12, EOnlineKeyValuePairDataType::String},
	{SEARCH_EXCLUDE_UNIQUEIDS, 			11, EOnlineKeyValuePairDataType::String},
	{SEARCH_USER, 						10, EOnlineKeyValuePairDataType::String},
	{SEARCH_KEYWORDS, 					9, EOnlineKeyValuePairDataType::String},
	{SEARCH_MATCHMAKING_QUEUE, 			8, EOnlineKeyValuePairDataType::String},
	{SEARCH_SWITCH_SELECTION_METHOD, 	7, EOnlineKeyValuePairDataType::String},
	{SEARCH_USER_ATTRIBUTE_TEAM, 		6, EOnlineKeyValuePairDataType::String},
	{SETTING_HOST_NICKNAME, 			5, EOnlineKeyValuePairDataType::String},

	// Number
	{SETTING_NUMBOTS, 					30, EOnlineKeyValuePairDataType::Int32},
	{SETTING_BEACONPORT, 				29, EOnlineKeyValuePairDataType::Int32},
	{SETTING_QOS, 						28, EOnlineKeyValuePairDataType::Int32},
	{SETTING_NEEDS, 					27, EOnlineKeyValuePairDataType::Int32},
	{SETTING_NEEDSSORT, 				26, EOnlineKeyValuePairDataType::Int32},
	{SETTING_MAXSPECTATORS, 			25, EOnlineKeyValuePairDataType::Int32},
	{SETTING_MATCHING_TIMEOUT, 			24, EOnlineKeyValuePairDataType::Int32},
	{SETTING_MAX_RESULT,				23, EOnlineKeyValuePairDataType::Int32},
	{SETTING_CONTRACT_VERSION_FILTER,	22, EOnlineKeyValuePairDataType::Int32},
	{SETTING_FIND_PRIVATE_SESSIONS,		21, EOnlineKeyValuePairDataType::Int32},
	{SETTING_FIND_RESERVED_SESSIONS, 	20, EOnlineKeyValuePairDataType::Int32},
	{SETTING_FIND_INACTIVE_SESSIONS,	19, EOnlineKeyValuePairDataType::Int32},
	{SETTING_MULTIPLAYER_VISIBILITY,	18, EOnlineKeyValuePairDataType::Int32},
	{SETTING_CHANGE_NUMBER,				17, EOnlineKeyValuePairDataType::Int32},
	{SETTING_ALLOW_ARBITER_MIGRATION,	16, EOnlineKeyValuePairDataType::Int32},
	{SETTING_MATCHING_PRESERVE_SESSION,	15, EOnlineKeyValuePairDataType::Int32},
	{SEARCH_MINSLOTSAVAILABLE,			14, EOnlineKeyValuePairDataType::Int32},
	{SETTING_BROADCASTER, 				13, EOnlineKeyValuePairDataType::Bool},
	{SETTING_PARTYMEMBERSPECTATOR, 		12, EOnlineKeyValuePairDataType::Bool},
	{SETTING_PARTY_ENABLED_SESSION, 	11, EOnlineKeyValuePairDataType::Bool},
	{SEARCH_EMPTY_SERVERS_ONLY, 		10, EOnlineKeyValuePairDataType::Bool},
	{SEARCH_NONEMPTY_SERVERS_ONLY, 		9, EOnlineKeyValuePairDataType::Bool},
	{SEARCH_SECURE_SERVERS_ONLY, 		8, EOnlineKeyValuePairDataType::Bool},
	{SEARCH_PRESENCE, 					7, EOnlineKeyValuePairDataType::Bool},
	{SEARCH_LOBBIES, 					6, EOnlineKeyValuePairDataType::Bool},
};

FPlayFabLobby::FPlayFabLobby(FOnlineSubsystemPlayFab* InOSSPlayFab) :
	OSSPlayFab(InOSSPlayFab)
{
	BuildSearchKeyMappingTable();
}

bool FPlayFabLobby::CreatePlayFabLobby(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	return CreateLobbyWithUser(HostingPlayerId, SessionName, NewSessionSettings);
}

bool FPlayFabLobby::CreateLobbyWithUser(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	PFLobbyHandle LobbyHandle;
	PFLobbyCreateConfiguration LobbyCreateConfig{};
	PFLobbyJoinConfiguration LobbyJoinConfig{};

	auto SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
	FOnlineSessionSettings UpdateSessionSettings(NewSessionSettings);

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	if (!PlayFabIdentityInt)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::CreateLobbyWithUser: Identity Interface is invalid"));
		return false;
	}

	TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(HostingPlayerId);
	if (LocalUser == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::CreateLobbyWithUser: GetPartyLocalUserFromPlatformId returned empty user!"));
		return false;
	}

	FString PlayerNickName = PlayFabIdentityInt->GetPlayerNickname(HostingPlayerId);
	UpdateSessionSettings.Set(SETTING_HOST_NICKNAME, PlayerNickName, EOnlineDataAdvertisementType::ViaOnlineService);

	LobbyCreateConfig.maxMemberCount = UpdateSessionSettings.NumPublicConnections;
	LobbyCreateConfig.ownerMigrationPolicy = PFLobbyOwnerMigrationPolicy::Automatic;
	LobbyCreateConfig.accessPolicy = PFLobbyAccessPolicy::Private;
	if (UpdateSessionSettings.bShouldAdvertise)
	{
		if (UpdateSessionSettings.bAllowJoinViaPresenceFriendsOnly)
		{
			LobbyCreateConfig.accessPolicy = PFLobbyAccessPolicy::Friends;
		}
		else
		{
			LobbyCreateConfig.accessPolicy = PFLobbyAccessPolicy::Public;
		}
	}

	AnsiStringList MemberKeys, MemberValues;
	MemberKeys.Add(SETTING_PLATFORM_ID);
	MemberValues.Add(LocalUser->GetPlatformUserId());
	MemberKeys.Add(SETTING_PLATFORM_MODEL);
	MemberValues.Add(PLATFORM_MODEL);

	LobbyJoinConfig.memberPropertyCount = MemberKeys.GetCount();
	LobbyJoinConfig.memberPropertyKeys = MemberKeys.GetData();
	LobbyJoinConfig.memberPropertyValues = MemberValues.GetData();

	AnsiStringList LobbyKeys, LobbyValues;
	AnsiStringList SearchKeys, SearchValues;

	// Set session custom settings
	for (FSessionSettings::TConstIterator It(UpdateSessionSettings.Settings); It; ++It)
	{
		const FName& SettingName = It.Key();
		const FOnlineSessionSetting& SettingValue = It.Value();
		const FString SettingNameString = SettingName.ToString();
		const FString SettingValueString = SettingValue.Data.ToString();

		// Only upload values that are marked for service use
		if (SettingValue.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			if (SettingValueString.IsEmpty())
			{
				UE_LOG_ONLINE(Warning, TEXT("CreateLobbyWithUser: %s: <Empty>."), *SettingNameString);
				LobbyKeys.Add(SettingNameString);
				LobbyValues.AddNull();
			}
			else
			{
				UE_LOG_ONLINE(Verbose, TEXT("CreateLobbyWithUser: %s: %s."), *SettingNameString, *SettingValueString);
				LobbyKeys.Add(SettingNameString);
				LobbyValues.Add(SettingValueString);
			}
		}

		// Add search attribute settings to lobby's search properties
		if (IsSearchKey(SettingNameString))
		{
			if (SettingValueString.IsEmpty())
			{
				UE_LOG_ONLINE(Warning, TEXT("CreateLobbyWithUser: %s: <Empty>."), *SettingNameString);
				SearchKeys.Add(SettingNameString);
				SearchValues.AddNull();
			}
			else
			{
				UE_LOG_ONLINE(Verbose, TEXT("CreateLobbyWithUser: %s: %s."), *SettingNameString, *SettingValueString);
				SearchKeys.Add(SettingNameString);
				SearchValues.Add(SettingValueString);
			}
		}
		else
		{
			FString SearchKey;
			EOnlineKeyValuePairDataType::Type Type;
			if (GetSearchKeyFromSettingMappingTable(SettingNameString, SearchKey, Type))
			{
				UE_LOG_ONLINE(Verbose, TEXT("CreateLobbyWithUser: predefined item %s(%s): %s Type: %d."), *SettingNameString, *SearchKey, *SettingValueString, Type);
				switch (Type)
				{
				case EOnlineKeyValuePairDataType::Bool:
				{
					const FVariantData& VariantData = SettingValue.Data;
					bool BoolVal;
					VariantData.GetValue(BoolVal);
					SearchKeys.Add(TCHAR_TO_ANSI(*SearchKey));
					SearchValues.Add(BoolVal == true ? "1" : "0");
					break;
				}
				case EOnlineKeyValuePairDataType::Int32:
				case EOnlineKeyValuePairDataType::String:
					SearchKeys.Add(TCHAR_TO_ANSI(*SearchKey));
					SearchValues.Add(SettingValueString);
					break;
				}
			}
		}
	}

	// Set our session setting bools
	{
		int32 BitShift = 0;
		int32 SessionSettingsFlags = 0;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bShouldAdvertise) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bAllowJoinInProgress) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bIsLANMatch) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bIsDedicated) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bUsesStats) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bAllowInvites) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bUsesPresence) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bAllowJoinViaPresence) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bAllowJoinViaPresenceFriendsOnly) << BitShift++;
		SessionSettingsFlags |= ((int32)UpdateSessionSettings.bAntiCheatProtected) << BitShift++;

		FString SessionSettingsFlagsName(TEXT("_flags"));
		const FString SessionSettingsFlagsValue(FString::FromInt(SessionSettingsFlags));

		UE_LOG_ONLINE_SESSION(Verbose, TEXT("Applying session settings flags: %s: %s."), *SessionSettingsFlagsName, *SessionSettingsFlagsValue);
		LobbyKeys.Add(SessionSettingsFlagsName);
		LobbyValues.Add(SessionSettingsFlagsValue);
	}


	LobbyCreateConfig.lobbyPropertyCount = LobbyKeys.GetCount();
	LobbyCreateConfig.lobbyPropertyKeys = LobbyKeys.GetData();
	LobbyCreateConfig.lobbyPropertyValues = LobbyValues.GetData();

	// Add remaining session search properties
	SearchKeys.Add(SEARCH_KEY_PLATFORM_ID);
	SearchValues.Add(LocalUser->GetPlatformUserId());
	SearchKeys.Add(SEARCH_KEY_HOST_NICKNAME);
	SearchValues.Add(PlayerNickName);

	LobbyCreateConfig.searchPropertyCount = SearchKeys.GetCount();
	LobbyCreateConfig.searchPropertyKeys = SearchKeys.GetData();
	LobbyCreateConfig.searchPropertyValues = SearchValues.GetData();

	PFEntityKey EntityKey = LocalUser->GetEntityKey();
	HRESULT Hr = PFMultiplayerCreateAndJoinLobby(OSSPlayFab->GetMultiplayerHandle(), &EntityKey, &LobbyCreateConfig, &LobbyJoinConfig, nullptr, &LobbyHandle);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFMultiplayerCreateAndJoinLobby failed: Error code [0x%08x] , Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		return false;
	}
	LobbySessionMap.Add(LobbyHandle, SessionName);

	return true;
}

bool FPlayFabLobby::JoinLobby(const FUniqueNetId& UserId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	return JoinLobbyWithUser(UserId, SessionName, DesiredSession.Session.SessionSettings);
}

bool FPlayFabLobby::JoinLobbyWithUser(const FUniqueNetId& UserId, FName SessionName, const FOnlineSessionSettings& SessionSettings)
{
	PFLobbyHandle LobbyHandle;
	PFLobbyJoinConfiguration LobbyConfig{};

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	if (!PlayFabIdentityInt)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::JoinLobbyWithUser: Identity Interface is invalid"));
		return false;
	}

	TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(UserId);
	if (LocalUser == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::JoinLobbyWithUser returned empty user!"));
		return false;
	}

	AnsiStringList MemberKeys, MemberValues;
	MemberKeys.Add(SETTING_PLATFORM_ID);
	MemberValues.Add(LocalUser->GetPlatformUserId());
	MemberKeys.Add(SETTING_PLATFORM_MODEL);
	MemberValues.Add(PLATFORM_MODEL);

	// TODO add the user settings based on Custom flags set by the game, similar to how GDK is doing it.
	for (FSessionSettings::TConstIterator It(SessionSettings.Settings); It; ++It)
	{
		const FName& SettingName = It.Key();
		const FOnlineSessionSetting& SettingValue = It.Value();

		// Only upload values that are marked for service use
		if (SettingValue.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			const FString SettingNameString = SettingName.ToString();
			const FString SettingValueString = SettingValue.Data.ToString();

			if (SettingValueString.IsEmpty())
			{
				UE_LOG_ONLINE(Warning, TEXT("JoinLobbyWithUser: %s: <Empty>."), *SettingNameString);
				// TODO: delete value from settings?
			}
			else
			{
				UE_LOG_ONLINE(Verbose, TEXT("JoinLobbyWithUser: %s: %s."), *SettingNameString, *SettingValueString);
				MemberKeys.Add(SettingNameString);
				MemberValues.Add(SettingValueString);
			}
		}
	}

	LobbyConfig.memberPropertyCount = MemberKeys.GetCount();
	LobbyConfig.memberPropertyKeys = MemberKeys.GetData();
	LobbyConfig.memberPropertyValues = MemberValues.GetData();

	FString ConnectionString;
	if ((SessionSettings.Get(SETTING_CONNECTION_STRING, ConnectionString)) == false)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::JoinLobbyWithUser no connection string available in session settings"));
		return false;
	}

	PFEntityKey EntityKey = LocalUser->GetEntityKey();
	HRESULT Hr = PFMultiplayerJoinLobby(OSSPlayFab->GetMultiplayerHandle(), &EntityKey, TCHAR_TO_UTF8(*ConnectionString), &LobbyConfig, nullptr, &LobbyHandle);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFMultiplayerJoinLobby failed: Error code [0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		return false;
	}
	LobbySessionMap.Add(LobbyHandle, SessionName);

	return true;
}

bool FPlayFabLobby::JoinArrangedLobby(FName SessionName, const FOnlineMatchmakingTicketInfoPtr MatchTicket)
{
	PFLobbyHandle LobbyHandle;
	PFLobbyArrangedJoinConfiguration LobbyConfig{};
	FString CustomMatchMemberKey = TEXT("CustomMatchJsonProperties");

	LobbyConfig.maxMemberCount = MatchTicket->PlayFabMatchmakingDetails->memberCount;
	LobbyConfig.ownerMigrationPolicy = PFLobbyOwnerMigrationPolicy::Automatic;
	LobbyConfig.accessPolicy = PFLobbyAccessPolicy::Private;

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	if (!PlayFabIdentityInt)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::JoinArrangedLobby: Identity Interface is invalid"));
		return false;
	}

	FUniqueNetIdPtr PlayerNativeNetId = OSSPlayFab->GetNativeNetId(MatchTicket->SearchingPlayerNetId->AsShared());
	FString PlayerNickName = PlayFabIdentityInt->GetPlayerNickname(*PlayerNativeNetId);

	AnsiStringList MemberKeys, MemberValues;
	MemberKeys.Add(SETTING_PLATFORM_ID);
	MemberValues.Add(MatchTicket->SearchingPlayerNetId->ToString());
	MemberKeys.Add(SETTING_PLATFORM_MODEL);
	MemberValues.Add(PLATFORM_MODEL);

	FString LocalEntityId = MatchTicket->GetHostUser()->GetEntityId();
	bool localMemberFound = false;

	// TODO should we update the MemberSettings with CustomMatchJsonProperties with key-value pairs for game to easily use it?
	for (uint32_t i = 0; i < MatchTicket->PlayFabMatchmakingDetails->memberCount; i++)
	{
		const PFMatchmakingMatchMember& Member = MatchTicket->PlayFabMatchmakingDetails->members[i];
		if (LocalEntityId.Compare(FString(Member.entityKey.id)) == 0)
		{
			MemberKeys.Add(CustomMatchMemberKey);
			MemberValues.Add(FString(Member.attributes));
			localMemberFound = true;
			// The attributes are json format, so only one value is possible.
			break;
		}
	}

	if (!localMemberFound)
	{
		UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::JoinArrangedLobby: could not find local member in match members!"));
	}

	LobbyConfig.memberPropertyCount = MemberKeys.GetCount();
	LobbyConfig.memberPropertyKeys = MemberKeys.GetData();
	LobbyConfig.memberPropertyValues = MemberValues.GetData();

	PFEntityKey EntityKey = MatchTicket->GetHostUser()->GetEntityKey();
	TUniquePtr<FString> LocalPlayerNickName = MakeUnique<FString>(PlayerNickName);
	HRESULT Hr = PFMultiplayerJoinArrangedLobby(OSSPlayFab->GetMultiplayerHandle(), &EntityKey, MatchTicket->PlayFabMatchmakingDetails->lobbyArrangementString, &LobbyConfig, static_cast<void*>(LocalPlayerNickName.Release()), &LobbyHandle);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFMultiplayerJoinArrangedLobby failed: Error code [0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		return false;
	}
	LobbySessionMap.Add(LobbyHandle, SessionName);

	return true;
}

bool FPlayFabLobby::UpdateLobby(FName SessionName, const FOnlineSessionSettings& SessionSettings)
{
	int OperationId = NextUpdateLobbyOperationId;
	NextUpdateLobbyOperationId.fetch_add(1);

	PFLobbyHandle LobbyHandle = nullptr;
	if (!GetLobbyFromSession(SessionName, LobbyHandle))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::UpdateLobby: No lobby found for session %s!"), *(SessionName.ToString()));
		return false;
	}

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	if (!PlayFabIdentityInt)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::UpdateLobby: Identity Interface is invalid"));
		return false;
	}

	FUpdateLobbyCompletionState UpdateLobbyCompletionState;
	UpdateLobbyCompletionState.LobbyPostUpdateCount = 0;
	UpdateLobbyCompletionState.MergedCompletionResult = true;

	// Update member properties for all party local users
	const TArray<TSharedPtr<FPlayFabUser>>& PartyLocalUsers = PlayFabIdentityInt->GetAllPartyLocalUsers();
	for (TSharedPtr<FPlayFabUser> User : PartyLocalUsers)
	{
		if (User.IsValid())
		{
			if (FSessionSettings* UpdatedMemberSettings = (FSessionSettings*)SessionSettings.MemberSettings.Find(FUniqueNetIdPlayFab::Create(User->GetPlatformUserId())))
			{
				AnsiStringList MemberKeys, MemberValues;

				for (FSessionSettings::TIterator It = UpdatedMemberSettings->CreateIterator(); It; ++It)
				{
					const FOnlineSessionSetting& SettingValue = It.Value();
					const FString SettingNameString = It.Key().ToString();
					const FString SettingValueString = SettingValue.Data.ToString();
					// Only upload values that are marked for service use
					if (SettingValue.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
					{
						if (SettingValueString.IsEmpty())
						{
							UE_LOG_ONLINE(Warning, TEXT("UpdateLobby Member Property: %s: <Empty>."), *SettingNameString);
							MemberKeys.Add(SettingNameString);
							MemberValues.AddNull();
						}
						else
						{
							UE_LOG_ONLINE(Verbose, TEXT("UpdateLobby Member Property: %s: %s."), *SettingNameString, *SettingValueString);
							MemberKeys.Add(SettingNameString);
							MemberValues.Add(SettingValueString);
						}
					}
				}

				PFEntityKey EntityKey = User->GetEntityKey();

				PFLobbyMemberDataUpdate MemberUpdateData{};
				MemberUpdateData.memberPropertyCount = MemberKeys.GetCount();
				MemberUpdateData.memberPropertyKeys = MemberKeys.GetData();
				MemberUpdateData.memberPropertyValues = MemberValues.GetData();

				UpdateLobbyCompletionState.LobbyPostUpdateCount++;
				TUniquePtr<TPair<int, int>> LobbyPostPair = MakeUnique<TPair<int, int>>(OperationId, UpdateLobbyCompletionState.LobbyPostUpdateCount);
				HRESULT Hr = PFLobbyPostUpdate(LobbyHandle, &EntityKey, nullptr, &MemberUpdateData, reinterpret_cast<void*>(LobbyPostPair.Release()));
				if (FAILED(Hr))
				{
					UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFLobbyPostUpdate update member properties failed. Error code [0x%08x]"), Hr);
					return false;
				}
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::UpdateLobbyMemberProperties: Failed to access member property for user %s"), *(User->GetPlatformUserId()));
			}
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::UpdateLobby: User is invalid"));
			return false;
		}
	}

	// Update lobby properties and search properties if we are the host
	const PFEntityKey* OwnerPtr;
	HRESULT Hr = PFLobbyGetOwner(LobbyHandle, &OwnerPtr);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::UpdateLobby failed to GetOwner: 0x%08x"), Hr);
		// Member properties were updated
		if (UpdateLobbyCompletionState.LobbyPostUpdateCount > 0)
		{
			UpdateLobbyOperations.Add(OperationId, UpdateLobbyCompletionState);
			return true;
		}
		return false;
	}

	if (OwnerPtr == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::UpdateLobby found no owner"));
		// Member properties were updated
		if (UpdateLobbyCompletionState.LobbyPostUpdateCount > 0)
		{
			UpdateLobbyOperations.Add(OperationId, UpdateLobbyCompletionState);
			return true;
		}
		return false;
	}

	if (!PlayFabIdentityInt->IsUserLocal(*OwnerPtr))
	{
		UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::UpdateLobby Owner of the lobby is not a local user!"));
		// Member properties were updated
		if (UpdateLobbyCompletionState.LobbyPostUpdateCount > 0)
		{
			UpdateLobbyOperations.Add(OperationId, UpdateLobbyCompletionState);
			return true;
		}
		return false;
	}

	AnsiStringList LobbyKeys, LobbyValues;
	AnsiStringList SearchKeys, SearchValues;

	// Set session custom settings
	for (FSessionSettings::TConstIterator It(SessionSettings.Settings); It; ++It)
	{
		const FName& SettingName = It.Key();
		const FOnlineSessionSetting& SettingValue = It.Value();
		const FString SettingNameString = SettingName.ToString();
		const FString SettingValueString = SettingValue.Data.ToString();

		// Only upload values that are marked for service use
		if (SettingValue.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			if (SettingValueString.IsEmpty())
			{
				UE_LOG_ONLINE(Warning, TEXT("UpdateLobby Lobby Property: %s: <Empty>."), *SettingNameString);
				LobbyKeys.Add(SettingNameString);
				LobbyValues.AddNull();
			}
			else
			{
				UE_LOG_ONLINE(Verbose, TEXT("UpdateLobby Lobby Property: %s: %s."), *SettingNameString, *SettingValueString);
				LobbyKeys.Add(SettingNameString);
				LobbyValues.Add(SettingValueString);
			}
		}

		// Add search attribute settings to lobby's search properties
		if (IsSearchKey(SettingNameString))
		{
			if (SettingValueString.IsEmpty())
			{
				UE_LOG_ONLINE(Warning, TEXT("UpdateLobby Search Property: %s: <Empty>."), *SettingNameString);
				SearchKeys.Add(SettingNameString);
				SearchValues.AddNull();
			}
			else
			{
				UE_LOG_ONLINE(Verbose, TEXT("UpdateLobby Search Property: %s: %s."), *SettingNameString, *SettingValueString);
				SearchKeys.Add(SettingNameString);
				SearchValues.Add(SettingValueString);
			}
		}
		else
		{
			FString SearchKey;
			EOnlineKeyValuePairDataType::Type Type;
			if (GetSearchKeyFromSettingMappingTable(SettingNameString, SearchKey, Type))
			{
				UE_LOG_ONLINE(Verbose, TEXT("UpdateLobby: predefined item %s(%s): %s Type: %d."), *SettingNameString, *SearchKey, *SettingValueString, Type);
				switch (Type)
				{
				case EOnlineKeyValuePairDataType::Bool:
				{
					const FVariantData& VariantData = SettingValue.Data;
					bool BoolVal;
					VariantData.GetValue(BoolVal);
					SearchKeys.Add(TCHAR_TO_ANSI(*SearchKey));
					SearchValues.Add(BoolVal == true ? "1" : "0");
					break;
				}
				case EOnlineKeyValuePairDataType::Int32:
				case EOnlineKeyValuePairDataType::String:
					SearchKeys.Add(TCHAR_TO_ANSI(*SearchKey));
					SearchValues.Add(SettingValueString);
					break;
				}
			}
		}
	}

	// Set our session setting bools
	{
		int32 BitShift = 0;
		int32 SessionSettingsFlags = 0;
		SessionSettingsFlags |= ((int32)SessionSettings.bShouldAdvertise) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bAllowJoinInProgress) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bIsLANMatch) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bIsDedicated) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bUsesStats) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bAllowInvites) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bUsesPresence) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bAllowJoinViaPresence) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bAllowJoinViaPresenceFriendsOnly) << BitShift++;
		SessionSettingsFlags |= ((int32)SessionSettings.bAntiCheatProtected) << BitShift++;

		FString SessionSettingsFlagsName(TEXT("_flags"));
		const FString SessionSettingsFlagsValue(FString::FromInt(SessionSettingsFlags));

		UE_LOG_ONLINE_SESSION(Verbose, TEXT("Applying session settings flags: %s: %s."), *SessionSettingsFlagsName, *SessionSettingsFlagsValue);
		LobbyKeys.Add(SessionSettingsFlagsName);
		LobbyValues.Add(SessionSettingsFlagsValue);
	}

	PFLobbyDataUpdate UpdateData = {};

	UpdateData.lobbyPropertyCount = LobbyKeys.GetCount();
	UpdateData.lobbyPropertyKeys = LobbyKeys.GetData();
	UpdateData.lobbyPropertyValues = LobbyValues.GetData();

	UpdateData.searchPropertyCount = SearchKeys.GetCount();
	UpdateData.searchPropertyKeys = SearchKeys.GetData();
	UpdateData.searchPropertyValues = SearchValues.GetData();

	UpdateLobbyCompletionState.LobbyPostUpdateCount++;
	TUniquePtr<TPair<int, int>> LobbyPostPair = MakeUnique<TPair<int, int>>(OperationId, UpdateLobbyCompletionState.LobbyPostUpdateCount);
	Hr = PFLobbyPostUpdate(LobbyHandle, OwnerPtr, &UpdateData, nullptr, reinterpret_cast<void*>(LobbyPostPair.Release()));
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFLobbyPostUpdate update lobby and search properties failed. Error code [0x%08x]"), Hr);
		return false;
	}

	UpdateLobbyOperations.Add(OperationId, UpdateLobbyCompletionState);
	return true;
}

bool FPlayFabLobby::AddLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate)
{	
	auto SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
	FNamedOnlineSessionPtr SessionPtr = SessionInterface->GetNamedSessionPtr(SessionName);
	if (!SessionPtr.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot add player to session %s, that session does not exist"), *SessionName.ToString());
		return false;
	}

	PFLobbyHandle LobbyHandle = nullptr;
	if (!GetLobbyFromSession(SessionName, LobbyHandle))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::AddLocalPlayer: No lobby found for session %s!"), *(SessionName.ToString()));
		return false;
	}

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::CreateAndJoinLobby Identity Interface is invalid"));
		return false;
	}

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());

	TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(PlayerId);
	if (LocalUser == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::GetPartyLocalUserFromPlatformId returned empty user!"));
		return false;
	}

	AnsiStringList MemberKeys, MemberValues;
	MemberKeys.Add(SETTING_PLATFORM_ID);
	MemberValues.Add(LocalUser->GetPlatformUserId());
	MemberKeys.Add(SETTING_PLATFORM_MODEL);
	MemberValues.Add(PLATFORM_MODEL);

	PFEntityKey JoiningEntityKey = LocalUser->GetEntityKey();
	HRESULT Hr = PFLobbyAddMember(LobbyHandle, &JoiningEntityKey, MemberKeys.GetCount(), MemberKeys.GetData(), MemberValues.GetData(), nullptr);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFLobbyAddMember failed: Error code [0x%08x] , Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		return false;
	}
		
	AddLocalPlayerData.PlayerPlatformId = LocalUser->GetPlatformUserId();
	AddLocalPlayerData.RegisterLocalPlayerCompleteDelegate = Delegate;

	return true;
}

bool FPlayFabLobby::LeaveLobby(const FUniqueNetId& PlayerId, FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate, const FOnUnregisterLocalPlayerCompleteDelegate& UnregisterLocalPlayerCompleteDelegate, bool bDestroyingSession)
{
	PFLobbyHandle LobbyHandle = nullptr;
	if (!GetLobbyFromSession(SessionName, LobbyHandle))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::LeaveLobby: No lobby found for session %s!"), *(SessionName.ToString()));
		return false;
	}

	HRESULT Hr;
	if (bDestroyingSession)
	{
		UE_LOG_ONLINE(Verbose, TEXT("FPlayFabLobby destroying session!"));
		Hr = PFLobbyLeave(LobbyHandle, nullptr, nullptr);
		if (FAILED(Hr))
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFLobbyLeave failed: 0x%08x"), Hr);
			CompletionDelegate.ExecuteIfBound(SessionName, false);
			return false;
		}
	}
	else
	{
		IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
		if (!IdentityIntPtr.IsValid())
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::LeaveLobby Identity Interface is invalid"));
			return false;
		}

		FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
		TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(PlayerId);
		if (LocalUser == nullptr)
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFLobbyLeave invalid PlayerId"));
			return false;
		}

		PFEntityKey LeavingEntityKey = LocalUser->GetEntityKey();
		Hr = PFLobbyLeave(LobbyHandle, &LeavingEntityKey, nullptr);
		if (FAILED(Hr))
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFLobbyLeave failed to UnregsterPlayer: 0x%08x"), Hr);
			return false;
		}
		RemoveLocalPlayerData.PlayerPlatformId = LocalUser->GetPlatformUserId();
		RemoveLocalPlayerData.UnregisterLocalPlayerCompleteDelegate = UnregisterLocalPlayerCompleteDelegate;
	}

	return true;
}

bool FPlayFabLobby::FindLobbies(const FUniqueNetId& UserId, TSharedPtr<FOnlineSessionSearch> SearchSettings)
{
	// Don't start another search while one is in progress
	if (!CurrentSessionSearch.IsValid() || SearchSettings->SearchState != EOnlineAsyncTaskState::InProgress)
	{
		// Copy the search pointer so we can keep it around
		CurrentSessionSearch = SearchSettings;
		SearchSettings->SearchState = EOnlineAsyncTaskState::InProgress;
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Ignoring Session Search request while one is pending."));
		return true;
	}

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::FindLobbies Identity Interface is invalid"));
		CurrentSessionSearch = nullptr;
		SearchSettings->SearchState = EOnlineAsyncTaskState::Failed;
		return false;
	}

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());

	TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(UserId);
	if (LocalUser == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::GetPartyLocalUserFromPlatformId returned empty user!"));
		CurrentSessionSearch = nullptr;
		SearchSettings->SearchState = EOnlineAsyncTaskState::Failed;
		return false;
	}
	
	// Get LocalUserNum for callback
	SearchingUserNum = PlayFabIdentityInt->GetPlatformUserIdFromUniqueNetId(UserId);
	
	PFLobbySearchConfiguration LobbySearchConfig{};
	FString QueryFilter;

	// Add lobby query filter string depending on search params specified in query settings
	QueryFilter = ComposeLobbySearchQueryFilter(SearchSettings->QuerySettings.SearchParams);
	std::string FilterString(TCHAR_TO_UTF8(*QueryFilter));
	if (!QueryFilter.IsEmpty())
	{
		LobbySearchConfig.filterString = FilterString.c_str();
	}

	PFEntityKey EntityKey = LocalUser->GetEntityKey();

	HRESULT Hr = PFMultiplayerFindLobbies(OSSPlayFab->GetMultiplayerHandle(), &EntityKey, &LobbySearchConfig, nullptr);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::FindLobbies failed: Error code [0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		CurrentSessionSearch = nullptr;
		SearchSettings->SearchState = EOnlineAsyncTaskState::Failed;
		return false;
	}

	return true;
}

bool FPlayFabLobby::FindFriendLobbies(const FUniqueNetId& UserId)
{
	// Don't start another search while one is in progress
	if (!CurrentSessionSearch.IsValid())
	{
		CurrentSessionSearch = MakeShareable(new FOnlineSessionSearch());
		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::InProgress;
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Ignoring find friend Session Search request while one is pending."));
		return true;
	}

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::FindFriendLobbies Identity Interface is invalid"));
		CurrentSessionSearch = nullptr;
		return false;
	}

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());

	TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(UserId);
	if (LocalUser == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::GetPartyLocalUserFromPlatformId returned empty user!"));
		CurrentSessionSearch = nullptr;
		return false;
	}

	// Get LocalUserNum for callback
	SearchingUserNum = PlayFabIdentityInt->GetPlatformUserIdFromUniqueNetId(UserId);

	// TODO Fill up with SearchSettings
	PFLobbySearchConfiguration LobbySearchConfig{};
	PFLobbySearchFriendsFilter LobbySearchFriendsFilter;

#if defined(OSS_PLAYFAB_GDK)
	LobbySearchFriendsFilter.includeFacebookFriends = false;
	LobbySearchFriendsFilter.includeSteamFriends = false;
	FString XToken = PlayFabIdentityInt->GetLocalUserXToken();
	std::string XTokenString(TCHAR_TO_UTF8(*XToken));
	LobbySearchFriendsFilter.includeXboxFriendsToken = XTokenString.c_str();
#endif

	LobbySearchConfig.friendsFilter = &LobbySearchFriendsFilter;

	PFEntityKey EntityKey = LocalUser->GetEntityKey();

	HRESULT Hr = PFMultiplayerFindLobbies(OSSPlayFab->GetMultiplayerHandle(), &EntityKey, &LobbySearchConfig, nullptr);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::FindFriendLobbies failed: Error code [0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		CurrentSessionSearch = nullptr;
		return false;
	}

	return true;
}

void FPlayFabLobby::RegisterForInvites_PlayFabMultiplayer(const PFEntityKey& ListenerEntity)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FPlayFabLobby::RegisterForInvites_PlayFabMultiplayer()"));

	HRESULT Hr = PFMultiplayerStartListeningForLobbyInvites(OSSPlayFab->GetMultiplayerHandle(), &ListenerEntity);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFMultiplayerStartListeningForLobbyInvites", Hr);
	}
}

void FPlayFabLobby::UnregisterForInvites_PlayFabMultiplayer(const PFEntityKey& ListenerEntity)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FPlayFabLobby::UnregisterForInvites_PlayFabMultiplayer()"));

	HRESULT Hr = PFMultiplayerStopListeningForLobbyInvites(OSSPlayFab->GetMultiplayerHandle(), &ListenerEntity);
	if (FAILED(Hr))
	{
		LogMultiplayerErrorWithMessage("PFMultiplayerStopListeningForLobbyInvites", Hr);
	}
}

void FPlayFabLobby::DoWork()
{
	uint32 StateChangeCount = 0u;
	const PFLobbyStateChange* const* StateChanges = nullptr;

	HRESULT Hr = PFMultiplayerStartProcessingLobbyStateChanges(OSSPlayFab->GetMultiplayerHandle(), &StateChangeCount, &StateChanges);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::DoWork::PFMultiplayerStartProcessingLobbyStateChanges failed. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		return;
	}

	for (uint32 i = 0; i < StateChangeCount; ++i)
	{
		const PFLobbyStateChange& StateChange = *StateChanges[i];

		switch (StateChange.stateChangeType)
		{
			case PFLobbyStateChangeType::CreateAndJoinLobbyCompleted:
			{				
				HandleCreateAndJoinLobbyCompleted(static_cast<const PFLobbyCreateAndJoinLobbyCompletedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::JoinLobbyCompleted:
			{
				HandleJoinLobbyCompleted(static_cast<const PFLobbyJoinLobbyCompletedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::MemberAdded:
			{
				HandleOnMemberAdded(static_cast<const PFLobbyMemberAddedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::AddMemberCompleted:
			{
				HandleOnAddMemberCompleted(static_cast<const PFLobbyAddMemberCompletedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::MemberRemoved:
			{
				HandleOnMemberRemoved(static_cast<const PFLobbyMemberRemovedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::ForceRemoveMemberCompleted:
			{
				HandleForceRemoveMember(static_cast<const PFLobbyForceRemoveMemberCompletedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::LeaveLobbyCompleted:
			{
				HandleLeaveLobbyCompleted(static_cast<const PFLobbyLeaveLobbyCompletedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::Updated:
			{
				HandleLobbyUpdate(static_cast<const PFLobbyUpdatedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::PostUpdateCompleted:
			{
				const auto& UpdateCompleted = static_cast<const PFLobbyPostUpdateCompletedStateChange&>(StateChange);
				TUniquePtr<TPair<int, int>> LobbyPostPair(reinterpret_cast<TPair<int, int>*> (UpdateCompleted.asyncContext));
				if (LobbyPostPair != nullptr)
				{
					int UpdateLobbyOperationId = LobbyPostPair->Key;
					FUpdateLobbyCompletionState* UpdateLobbyCompletionState = UpdateLobbyOperations.Find(UpdateLobbyOperationId);
					if (UpdateLobbyCompletionState == nullptr)
					{
						UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::DoWork received PFLobbyStateChangeType::PostUpdateCompleted, but UpdateLobbyOperations does not have valid key %d: "), LobbyPostPair->Key);
					}
					else
					{
						UpdateLobbyOperations[UpdateLobbyOperationId].MergedCompletionResult = UpdateLobbyCompletionState->MergedCompletionResult && SUCCEEDED(UpdateCompleted.result);
						// All PFLobbyPostUpdate calls in one UpdateLobby have been received
						if (UpdateLobbyCompletionState->LobbyPostUpdateCount == LobbyPostPair->Value)
						{
							FName* SessionName = LobbySessionMap.Find(UpdateCompleted.lobby);
							TriggerOnUpdateLobbyCompletedDelegates(*SessionName, UpdateLobbyOperations[UpdateLobbyOperationId].MergedCompletionResult);
							UpdateLobbyOperations.Remove(UpdateLobbyOperationId);

							// All UpdateLobby calls have been handled
							if (UpdateLobbyOperations.Num() == 0)
							{
								NextUpdateLobbyOperationId.store(0);
							}
						}
					}
				}
				else
				{
					UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::DoWork received PFLobbyStateChangeType::PostUpdateCompleted, and asyncContext is nullptr"));
				}
				break;
			}
			case PFLobbyStateChangeType::Disconnecting:
			{
				HandleLobbyDisconnecting(static_cast<const PFLobbyDisconnectingStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::Disconnected:
			{
				HandleLobbyDisconnected(static_cast<const PFLobbyDisconnectedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::JoinArrangedLobbyCompleted:
			{
				HandleJoinArrangedLobbyCompleted(static_cast<const PFLobbyJoinArrangedLobbyCompletedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::FindLobbiesCompleted:
			{
				HandleFindLobbiesCompleted(static_cast<const PFLobbyFindLobbiesCompletedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::InviteReceived:
			{
				HandleInvitationReceived(static_cast<const PFLobbyInviteReceivedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::InviteListenerStatusChanged:
			{
				HandleInviteListenerStatusChanged(static_cast<const PFLobbyInviteListenerStatusChangedStateChange&>(StateChange));
				break;
			}
			case PFLobbyStateChangeType::SendInviteCompleted:
			{
				HandleSendInviteCompleted(static_cast<const PFLobbySendInviteCompletedStateChange&>(StateChange));
				break;
			}
			default:
			{
				UE_LOG_ONLINE(Verbose, TEXT("Received PlayFab Unhandled(%u) event"), StateChange.stateChangeType);
				break;
			}
		}
	}
	Hr = PFMultiplayerFinishProcessingLobbyStateChanges(OSSPlayFab->GetMultiplayerHandle(), StateChangeCount, StateChanges);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::DoWork::PFMultiplayerFinishProcessingLobbyStateChanges failed. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
	}
}

void FPlayFabLobby::HandleCreateAndJoinLobbyCompleted(const PFLobbyCreateAndJoinLobbyCompletedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyCreateAndJoinLobbyCompletedStateChange(%u) event"), StateChange.stateChangeType);

	bool bSuccess = false;

	FName SessionName = NAME_None;

	if (FName* FoundSessionName = LobbySessionMap.Find(StateChange.lobby))
	{
		SessionName = *FoundSessionName;

		FOnlineSessionPlayFabPtr SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
		if (SessionInterface.IsValid())
		{
			FNamedOnlineSessionPtr ExistingNamedSession = SessionInterface->GetNamedSessionPtr(SessionName);
			if (ExistingNamedSession.IsValid())
			{
				const char* LobbyId;
				HRESULT Hr = PFLobbyGetLobbyId(StateChange.lobby, &LobbyId);
				if (SUCCEEDED(Hr))
				{
					const char* ConnectionString;
					Hr = PFLobbyGetConnectionString(StateChange.lobby, &ConnectionString);
					if (SUCCEEDED(Hr))
					{
						FOnlineSessionInfoPlayFabPtr NewSessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(ExistingNamedSession->SessionInfo);
						if (NewSessionInfo.IsValid())
						{
							bSuccess = true;

							NewSessionInfo->LobbyHandle = StateChange.lobby;
							NewSessionInfo->SetSessionId(ANSI_TO_TCHAR(LobbyId));
							NewSessionInfo->ConnectionString = ANSI_TO_TCHAR(ConnectionString);

							ExistingNamedSession->SessionState = EOnlineSessionState::Pending;

#if defined(OSS_PLAYFAB_GDK)
							ExistingNamedSession->SessionInfo = NewSessionInfo;
							SessionInterface->SetMultiplayerActivityForSession(ExistingNamedSession);
#endif
						}
						else
						{
							UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnCreateAndJoinLobbyCompleted: SessionInfo was null"));
						}
					}
					else
					{
						UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted: failed to GetConnectionString: 0x%08x"), Hr);
					}
				}
				else
				{
					UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted: failed to GetLobbyId: 0x%08x"), Hr);
				}
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted: ExistingNamedSession was null"));
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted: SessionInterface was null"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted: could not find session name from lobby handle"));
	}

	TriggerOnLobbyCreatedAndJoinCompletedDelegates(bSuccess, SessionName);
}

void FPlayFabLobby::HandleJoinLobbyCompleted(const PFLobbyJoinLobbyCompletedStateChange& StateChange)
{
	EOnJoinSessionCompleteResult::Type JoinResult = EOnJoinSessionCompleteResult::UnknownError;
	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyJoinLobbyCompletedStateChange(%u) event"), StateChange.stateChangeType);

	FName* SessionName = LobbySessionMap.Find(StateChange.lobby);
	if (FAILED(StateChange.result))
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to join lobby. ErrorCode=[0x%08x]"), StateChange.result);
	}
	else
	{
		JoinResult = EOnJoinSessionCompleteResult::Success;

		const PFEntityKey* OwnerEntityKeyPtr;
		HRESULT Hr = PFLobbyGetOwner(StateChange.lobby, &OwnerEntityKeyPtr);
		if (SUCCEEDED(Hr))
		{
			if (OwnerEntityKeyPtr == nullptr)
			{
				UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleJoinLobbyCompleted found no owner"));
			}
			else
			{
				auto SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
				SessionInterface->SetHostOnSession(*SessionName, *OwnerEntityKeyPtr);
			}
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleJoinLobbyCompleted failed to GetOwner for the lobby. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
		}
	}

	auto SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
	FNamedOnlineSessionPtr ExistingNamedSession = SessionInterface->GetNamedSessionPtr(*SessionName);
	ExistingNamedSession->bHosting = false; // you are never hosting when you join

	const char* LobbyId;
	HRESULT Hr = PFLobbyGetLobbyId(StateChange.lobby, &LobbyId);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted failed to GetLobbyId: 0x%08x"), Hr);
	}

	const char* ConnectionString;
	Hr = PFLobbyGetConnectionString(StateChange.lobby, &ConnectionString);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted failed to GetConnectionString: 0x%08x"), Hr);
	}

	TSharedRef<FOnlineSessionInfoPlayFab> NewSessionInfo = MakeShared<FOnlineSessionInfoPlayFab>();
	NewSessionInfo->SetSessionId(ANSI_TO_TCHAR(LobbyId));
	NewSessionInfo->ConnectionString = ANSI_TO_TCHAR(ConnectionString);
	NewSessionInfo->LobbyHandle = StateChange.lobby;
	NewSessionInfo->SessionName = *SessionName;

	ExistingNamedSession->SessionInfo = NewSessionInfo;
	ExistingNamedSession->SessionState = EOnlineSessionState::Pending;

#if defined(OSS_PLAYFAB_GDK)
	SessionInterface->SetMultiplayerActivityForSession(ExistingNamedSession);
#endif

	TriggerOnJoinLobbyCompletedDelegates(*SessionName, JoinResult);
}

void FPlayFabLobby::HandleJoinArrangedLobbyCompleted(const PFLobbyJoinArrangedLobbyCompletedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyJoinArrangedLobbyCompletedStateChange(%u) event"), StateChange.stateChangeType);
	bool bHosting = false;

	FName* SessionName = LobbySessionMap.Find(StateChange.lobby);
	if (FAILED(StateChange.result))
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to join arranged lobby. ErrorCode=[0x%08x]"), StateChange.result);
		TriggerOnJoinArrangedLobbyCompletedDelegates(*SessionName, false);
		return;
	}

	auto SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
	FNamedOnlineSessionPtr ExistingNamedSession = SessionInterface->GetNamedSessionPtr(*SessionName);

	const PFEntityKey* OwnerPtr;
	HRESULT Hr = PFLobbyGetOwner(StateChange.lobby, &OwnerPtr);
	if (SUCCEEDED(Hr))
	{
		if (OwnerPtr == nullptr)
		{
			UE_LOG_ONLINE(Error, TEXT("The owner does not exist!"));
			TriggerOnJoinArrangedLobbyCompletedDelegates(*SessionName, false);
			return;
		}

		IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
		if (!IdentityIntPtr.IsValid())
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleJoinArrangedLobbyCompleted Identity Interface is invalid"));
			TriggerOnJoinArrangedLobbyCompletedDelegates(*SessionName, false);
			return;
		}

		FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
		if (!PlayFabIdentityInt->IsUserLocal(*OwnerPtr))
		{
			UE_LOG_ONLINE(Verbose, TEXT("FPlayFabLobby::HandleJoinArrangedLobbyCompleted Owner of the lobby is not a local user!"));
		}
		else
		{
			UE_LOG_ONLINE(Verbose, TEXT("FPlayFabLobby::HandleJoinArrangedLobbyCompleted Owner of the lobby is a local user."));
			bHosting = true;
			FString PlayerNickName = *(static_cast<FString*>(StateChange.asyncContext));
			ExistingNamedSession->SessionSettings.Set(SETTING_HOST_NICKNAME, PlayerNickName, EOnlineDataAdvertisementType::ViaOnlineService);
			UpdateLobby(*SessionName, ExistingNamedSession->SessionSettings);
		}
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleJoinArrangedLobbyCompleted failed to GetOwner for the lobby. ErrorCode=[0x%08x], Error message:%s"), Hr, *GetMultiplayerErrorMessage(Hr));
	}

	ExistingNamedSession->bHosting = bHosting;

	const char* LobbyId;
	Hr = PFLobbyGetLobbyId(StateChange.lobby, &LobbyId);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted failed to GetLobbyId: 0x%08x"), Hr);
	}

	const char* ConnectionString;
	Hr = PFLobbyGetConnectionString(StateChange.lobby, &ConnectionString);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleCreateAndJoinLobbyCompleted failed to GetConnectionString: 0x%08x"), Hr);
	}

	TSharedRef<FOnlineSessionInfoPlayFab> NewSessionInfo = MakeShared<FOnlineSessionInfoPlayFab>();
	NewSessionInfo->SetSessionId(ANSI_TO_TCHAR(LobbyId));
	NewSessionInfo->ConnectionString = ANSI_TO_TCHAR(ConnectionString);
	NewSessionInfo->LobbyHandle = StateChange.lobby;
	NewSessionInfo->SessionName = *SessionName;

	ExistingNamedSession->SessionInfo = NewSessionInfo;
	ExistingNamedSession->SessionState = EOnlineSessionState::Pending;

#if defined(OSS_PLAYFAB_GDK)
	SessionInterface->SetMultiplayerActivityForSession(ExistingNamedSession);
#endif

	TriggerOnJoinArrangedLobbyCompletedDelegates(*SessionName, true);
}

void FPlayFabLobby::HandleLobbyUpdate(const PFLobbyUpdatedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyUpdatedStateChange(%u) event"), StateChange.stateChangeType);

	FName* SessionName = LobbySessionMap.Find(StateChange.lobby);
	TriggerOnLobbyUpdateDelegates(*SessionName, StateChange);
}

void FPlayFabLobby::HandleOnMemberAdded(const PFLobbyMemberAddedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyMemberAddedStateChange(%u) event"), StateChange.stateChangeType);

	FName* SessionName = LobbySessionMap.Find(StateChange.lobby);
	TriggerOnLobbyMemberAddedDelegates(*SessionName, StateChange);
}

void FPlayFabLobby::HandleOnAddMemberCompleted(const PFLobbyAddMemberCompletedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyAddMemberCompletedStateChange(%u) event"), StateChange.stateChangeType);
	EOnJoinSessionCompleteResult::Type Result;
	if (SUCCEEDED(StateChange.result))
	{
		Result = EOnJoinSessionCompleteResult::Success;
	}
	else
	{
		Result = EOnJoinSessionCompleteResult::UnknownError;
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::HandleOnAddMemberCompleted failed with ErrorCode=[0x%08x], Error message:%s"), StateChange.result, *GetMultiplayerErrorMessage(StateChange.result));
	}
	AddLocalPlayerData.RegisterLocalPlayerCompleteDelegate.ExecuteIfBound(*FUniqueNetIdPlayFab::Create(AddLocalPlayerData.PlayerPlatformId), Result);
}

void FPlayFabLobby::HandleOnMemberRemoved(const PFLobbyMemberRemovedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyMemberRemovedStateChange(%u) event"), StateChange.stateChangeType);

	FName* SessionName = LobbySessionMap.Find(StateChange.lobby);
	TriggerOnLobbyMemberRemovedDelegates(*SessionName, StateChange);
}

void FPlayFabLobby::HandleForceRemoveMember(const PFLobbyForceRemoveMemberCompletedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("Received ForceRemoveMemberCompleted(%u) event"), StateChange.stateChangeType);
}

void FPlayFabLobby::HandleLeaveLobbyCompleted(const PFLobbyLeaveLobbyCompletedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("HandleLeaveLobbyCompleted: lobby: 0x%p"), StateChange.lobby);
	UE_LOG_ONLINE(Verbose, TEXT("HandleLeaveLobbyCompleted: localUser: 0x%p"), StateChange.localUser);
	UE_LOG_ONLINE(Verbose, TEXT("HandleLeaveLobbyCompleted: asyncContext: 0x%p"), StateChange.asyncContext);

	if (StateChange.lobby != nullptr)
	{
		FName* SessionName = LobbySessionMap.Find(StateChange.lobby);
		auto SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
		FNamedOnlineSessionPtr ExistingNamedSession = SessionInterface->GetNamedSessionPtr(*SessionName);

#if defined(OSS_PLAYFAB_GDK)
		//this session shouldn't have an activity to delete
		if (ExistingNamedSession->SessionSettings.bUsesPresence)
		{
			//If this value is null it signifies that the title requested all local members leave the specified lobby.
			if (StateChange.localUser)
			{
				SessionInterface->DeleteMultiplayerActivity(StateChange.lobby, *StateChange.localUser, ExistingNamedSession->SessionSettings);
			}
			else
			{
				//Delete the activity for all local users
				FOnlineIdentityPlayFabPtr PlayFabIdentityInt = OSSPlayFab ? OSSPlayFab->GetIdentityInterfacePlayFab() : nullptr;
				if (PlayFabIdentityInt.IsValid())
				{
					SessionInterface->DeleteMultiplayerActivity(StateChange.lobby, PlayFabIdentityInt->GetLocalUserEntityKeys(), ExistingNamedSession->SessionSettings);
				}
				else
				{
					UE_LOG_ONLINE_SESSION(Warning, TEXT("FOnlineSessionPlayFab::OnLeaveLobbyCompleted: Identity Interface is invalid"));
				}
			}
		}
#endif

		// Check if this is DestroySession or UnregisterLocalUser
		if (ExistingNamedSession->SessionState == EOnlineSessionState::Destroying)
		{
			SessionInterface->RemoveNamedSession(*SessionName);
			LobbySessionMap.Remove(StateChange.lobby);
			TriggerOnLeaveLobbyCompletedDelegates(*SessionName, true);
		}
		else
		{
			RemoveLocalPlayerData.UnregisterLocalPlayerCompleteDelegate.ExecuteIfBound(*FUniqueNetIdPlayFab::Create(RemoveLocalPlayerData.PlayerPlatformId), true);
		}
	}
	else
	{
		TriggerOnLeaveLobbyCompletedDelegates(FName(), false);
		RemoveLocalPlayerData.UnregisterLocalPlayerCompleteDelegate.ExecuteIfBound(*FUniqueNetIdPlayFab::Create(""), false);
	}
}

void FPlayFabLobby::HandleFindLobbiesCompleted(const PFLobbyFindLobbiesCompletedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("HandleFindLobbiesCompleted: result: 0x%08x"), StateChange.result);

	if (FAILED(StateChange.result))
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to find lobbies. ErrorCode=[0x%08x]"), StateChange.result);
		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
	}
	else
	{
		for (uint32_t i = 0; i < StateChange.searchResultCount; i++)
		{
			FString ConnectionString;
			FOnlineSessionSearchResult SearchResult = CreateSearchResultFromLobby(StateChange.searchResults[i]);
			if ((SearchResult.Session.SessionSettings.Get(SETTING_CONNECTION_STRING, ConnectionString)) == true && !ConnectionString.IsEmpty())
			{
				CurrentSessionSearch->SearchResults.Add(SearchResult);
			}
		}

		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Done;
	}

	TriggerOnFindLobbiesCompletedDelegates(SearchingUserNum, SUCCEEDED(StateChange.result), CurrentSessionSearch);
}

void FPlayFabLobby::HandleSendInviteCompleted(const PFLobbySendInviteCompletedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("HandleSendInviteCompleted: result: 0x%08x"), StateChange.result);
	UE_LOG_ONLINE(Verbose, TEXT("HandleSendInviteCompleted: lobby: 0x%p"), StateChange.lobby);
	UE_LOG_ONLINE(Verbose, TEXT("HandleSendInviteCompleted: sender: %s"), *FString(StateChange.sender.id));
	UE_LOG_ONLINE(Verbose, TEXT("HandleSendInviteCompleted: invitee: %s"), *FString(StateChange.invitee.id));

	// The Engine does not provide any delegate to fire for failed Invites hence the logs are for diagnostic purpose
}

void FPlayFabLobby::HandleInviteListenerStatusChanged(const PFLobbyInviteListenerStatusChangedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("HandleInviteListenerStatusChanged: listener: %s"), *FString(StateChange.listeningEntity.id));

	PFLobbyInviteListenerStatus Status;
	HRESULT Hr = PFMultiplayerGetLobbyInviteListenerStatus(OSSPlayFab->GetMultiplayerHandle(), &StateChange.listeningEntity, &Status);
	if (SUCCEEDED(Hr))
	{
		UE_LOG_ONLINE(Verbose, TEXT("Invitation listener status: %u"), Status);
	}
}

void FPlayFabLobby::HandleInvitationReceived(const PFLobbyInviteReceivedStateChange& StateChange)
{
	FOnlineSessionPlayFabPtr SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
	if (SessionInterface.IsValid())
	{
		SessionInterface->OnInvitationReceived_PlayFabMultiplayer(StateChange.listeningEntity, StateChange.invitingEntity, FString(StateChange.connectionString));
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("FPlayFabLobby::HandleInvitationReceived: SessionInterface was null"));
	}
}

void FPlayFabLobby::HandleLobbyDisconnected(const PFLobbyDisconnectedStateChange& StateChange)
{
	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyDisconnectedStateChange(%u) event"), StateChange.stateChangeType);
	FName* SessionName = LobbySessionMap.Find(StateChange.lobby);
	if (SessionName != nullptr)
	{
		LobbySessionMap.Remove(StateChange.lobby);
		TriggerOnLobbyDisconnectedDelegates(*SessionName);
	}
	else
	{
		TriggerOnLobbyDisconnectedDelegates(FName());
	}
}

void FPlayFabLobby::HandleLobbyDisconnecting(const PFLobbyDisconnectingStateChange& StateChange)
{
	FString DisconnectingReason;
	switch (StateChange.reason)
		{
			case PFLobbyDisconnectingReason::NoLocalMembers:
			{
				DisconnectingReason = "NoLocalMembers";
				break;
			}
			case PFLobbyDisconnectingReason::LobbyDeleted:
			{
				DisconnectingReason = "LobbyDeleted";
				break;
			}
			case PFLobbyDisconnectingReason::ConnectionInterruption:
			{
				DisconnectingReason = "ConnectionInterruption";
				break;
			}
		}

	UE_LOG_ONLINE(Verbose, TEXT("Received PFLobbyDisconnectingStateChange(%u) due to: %s"), StateChange.stateChangeType, *DisconnectingReason);
}

bool FPlayFabLobby::GetLobbyFromSession(const FName SessionName, PFLobbyHandle& LobbyHandle)
{
	for (auto Iterator : LobbySessionMap)
	{
		if (Iterator.Value == SessionName)
		{
			LobbyHandle = Iterator.Key;
			return true;
		}
	}

	UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::GetLobbyFromSession failed to find the lobby for session %s!"), *(SessionName.ToString()));
	return false;
}

bool FPlayFabLobby::ValidateSessionForInvite(const FName SessionName)
{
	auto SessionInterface = OSSPlayFab->GetSessionInterfacePlayFab();
	FNamedOnlineSessionPtr SessionPtr = SessionInterface->GetNamedSessionPtr(SessionName);
	if (!SessionPtr.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot Invite Friends to session %s, that session does not exist"), *SessionName.ToString());
		return false;
	}

	if (SessionPtr->SessionState < EOnlineSessionState::Pending || SessionPtr->SessionState > EOnlineSessionState::InProgress)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot Invite Friends to session %s, that session is in state %d"), *SessionName.ToString(), SessionPtr->SessionState);
		return false;
	}

	FOnlineSessionInfoPlayFabPtr SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(SessionPtr->SessionInfo);
	if (!SessionInfo.IsValid())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot Invite Friends to session %s, that session has invalid info"), *SessionName.ToString());
		return false;
	}
	return true;
}

FOnlineSessionSearchResult FPlayFabLobby::CreateSearchResultFromLobby(const PFLobbySearchResult& LobbySearchResult)
{
	UE_LOG_ONLINE(Verbose, TEXT("FPlayFabLobby::CreateSearchResultFromLobby"));

	FOnlineSessionSearchResult NewSearchResult;
	FString LobbyConnectionString;
	if (LobbySearchResult.connectionString != nullptr)
	{
		LobbyConnectionString = LobbySearchResult.connectionString;
		NewSearchResult.Session.SessionSettings.bAllowJoinInProgress = true;
	}
	else
	{
		NewSearchResult.Session.SessionSettings.bAllowJoinInProgress = false;
	}

	TSharedRef<FOnlineSessionInfoPlayFab> NewSessionInfo = MakeShared<FOnlineSessionInfoPlayFab>();
	NewSessionInfo->SetSessionId(ANSI_TO_TCHAR(LobbySearchResult.lobbyId));
	NewSessionInfo->ConnectionString = LobbyConnectionString;

	NewSearchResult.Session.SessionInfo = NewSessionInfo;
	NewSearchResult.Session.SessionSettings.Set(SETTING_CONNECTION_STRING, LobbyConnectionString, EOnlineDataAdvertisementType::ViaOnlineService);

	// Store lobby owner's platform Id and user name in search results
	bool PlatformIdKeyFound = false;
	bool OwnerNicknameFound = false;
#if defined(USES_NATIVE_SESSION)
	bool NativeSessionIdFound = false;
	bool NativePlatformFound = false;
#endif
	for (uint32_t i = 0; i < LobbySearchResult.searchPropertyCount; i++)
	{
		FString SearchPropertyKey = LobbySearchResult.searchPropertyKeys[i];
		if (SearchPropertyKey.Equals(SEARCH_KEY_PLATFORM_ID))
		{
			NewSearchResult.Session.OwningUserId = FUniqueNetIdPlayFab::Create(FString(LobbySearchResult.searchPropertyValues[i]));
			PlatformIdKeyFound = true;
			continue;
		}

		if (SearchPropertyKey.Equals(SEARCH_KEY_HOST_NICKNAME))
		{
			NewSearchResult.Session.OwningUserName = FString(LobbySearchResult.searchPropertyValues[i]);
			OwnerNicknameFound = true;
			continue;
		}
#if defined(USES_NATIVE_SESSION)
		if (SearchPropertyKey.Equals(SEARCH_KEY_NATIVE_SESSIONID))
		{
			FString NativeSessionIdString = FString(LobbySearchResult.searchPropertyValues[i]);
			NewSessionInfo->SetNativeSessionIdString(NativeSessionIdString);
			NativeSessionIdFound = true;
			continue;
		}
		if (SearchPropertyKey.Equals(SEARCH_KEY_NATIVE_PLATFORM))
		{
			FString NativePlatform = FString(LobbySearchResult.searchPropertyValues[i]);
			NewSessionInfo->SetNativePlatform(NativePlatform);
			NativePlatformFound = true;
			continue;
		}
#endif

		// return search properties back to session settings
		auto SettingKey = SearchKeyMappingTable.Find(SearchPropertyKey);
		if (SettingKey)
		{
			switch (SettingKey->Value)
			{
				case EOnlineKeyValuePairDataType::Bool:
					NewSearchResult.Session.SessionSettings.Set(FName(SettingKey->Key), LobbySearchResult.searchPropertyValues[i][0] == '1' ? true : false, EOnlineDataAdvertisementType::ViaOnlineService);
					break;
				case EOnlineKeyValuePairDataType::Int32:
					NewSearchResult.Session.SessionSettings.Set(FName(SettingKey->Key), FCStringAnsi::Atoi(LobbySearchResult.searchPropertyValues[i]), EOnlineDataAdvertisementType::ViaOnlineService);
					break;
				case EOnlineKeyValuePairDataType::String:
					NewSearchResult.Session.SessionSettings.Set(FName(SettingKey->Key), FString(LobbySearchResult.searchPropertyValues[i]), EOnlineDataAdvertisementType::ViaOnlineService);
					break;
			}
		}
		else
		{
			NewSearchResult.Session.SessionSettings.Set(FName(SearchPropertyKey), FString(LobbySearchResult.searchPropertyValues[i]), EOnlineDataAdvertisementType::ViaOnlineService);
		}
	}

	if (!PlatformIdKeyFound)
	{
		UE_LOG_ONLINE_SESSION(Error, TEXT("Platform Id key is not found in lobby search properties, lobby Id %s"), LobbySearchResult.lobbyId);
	}

	if (!OwnerNicknameFound)
	{
		UE_LOG_ONLINE_SESSION(Error, TEXT("Owner nickname is not found in lobby search properties, lobby Id %s"), LobbySearchResult.lobbyId);
	}

#if defined(USES_NATIVE_SESSION)
	if (!NativeSessionIdFound)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Native Session Id is not found in lobby search properties, lobby Id %s"), LobbySearchResult.lobbyId);
	}
	if (!NativePlatformFound)
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Native Platform is not found in lobby search properties, lobby Id %s"), LobbySearchResult.lobbyId);
	}
#endif

	NewSearchResult.Session.NumOpenPrivateConnections = 0;
	NewSearchResult.Session.NumOpenPublicConnections = LobbySearchResult.maxMemberCount - LobbySearchResult.currentMemberCount;
	NewSearchResult.Session.SessionSettings.NumPublicConnections = LobbySearchResult.maxMemberCount;
	return NewSearchResult;
}

bool FPlayFabLobby::IsSearchKey(const FString& Name)
{
	return Name.Equals(PFLobbyMemberCountSearchKey) || Name.Equals(PFLobbyAmMemberSearchKey) || Name.StartsWith(SEARCH_KEY_PREFIX_STRING) || Name.StartsWith(SEARCH_KEY_PREFIX_NUMBER);
}

FString FPlayFabLobby::ComposeLobbySearchQueryFilter(const FSearchParams& SearchParams)
{
	FString QueryFilter;
	for (auto SearchParam : SearchParams)
	{
		const FString& SettingName = SearchParam.Key.ToString();
		const FVariantData& SettingValue = SearchParam.Value.Data;
		if (SettingValue.ToString().IsEmpty() || SettingName == SEARCH_PRESENCE.ToString() || SettingName == SEARCH_LOBBIES.ToString())
		{
			continue;
		}

		if (!QueryFilter.IsEmpty())
		{
			QueryFilter.Append(TEXT(" and "));
		}

		EOnlineComparisonOp::Type ComparisonOp = SearchParam.Value.ComparisonOp;
		FString ComparisonString;
		switch (ComparisonOp)
		{
			case EOnlineComparisonOp::Equals:
			{
				ComparisonString = TEXT("eq");
				break;
			}

			case EOnlineComparisonOp::NotEquals:
			{
				ComparisonString = TEXT("ne");
				break;
			}

			case EOnlineComparisonOp::GreaterThanEquals:
			{
				ComparisonString = TEXT("ge");
				break;
			}

			case EOnlineComparisonOp::GreaterThan:
			{
				ComparisonString = TEXT("gt");
				break;
			}

			case EOnlineComparisonOp::LessThanEquals:
			{
				ComparisonString = TEXT("le");
				break;
			}

			case EOnlineComparisonOp::LessThan:
			{
				ComparisonString = TEXT("lt");
				break;
			}

			default:
			{
				ComparisonString = TEXT("eq");
				UE_LOG_ONLINE(Warning, TEXT("Comparison Operation %s not supported in PlayFab lobby queries. Defaulting to EQ"), EOnlineComparisonOp::ToString(ComparisonOp));
				break;
			}
		}

		FString SearchKey;
		EOnlineKeyValuePairDataType::Type Type;
		if (GetSearchKeyFromSettingMappingTable(SettingName, SearchKey, Type))
		{
			UE_LOG_ONLINE(Verbose, TEXT("ComposeLobbySearchQueryFilter: predefined item %s(%s): %s Type: %d."), *SettingName, *SearchKey, *SettingValue.ToString(), Type);
			switch (Type)
			{
			case EOnlineKeyValuePairDataType::Int32:
				QueryFilter.Append(FString::Printf(TEXT("%s %s %s"), *SearchKey, *ComparisonString, *SettingValue.ToString()));
				break;
			case EOnlineKeyValuePairDataType::Bool:
			{
				bool BoolVal;
				SettingValue.GetValue(BoolVal);
				QueryFilter.Append(FString::Printf(TEXT("%s %s %d"), *SearchKey, *ComparisonString, static_cast<int32>(BoolVal)));
				break;
			}
			case EOnlineKeyValuePairDataType::String:
				QueryFilter.Append(FString::Printf(TEXT("%s %s '%s'"), *SearchKey, *ComparisonString, *SettingValue.ToString()));
				break;
			}
		}
		else if (IsSearchKey(SettingName))
		{
			if (SettingValue.IsNumeric())
			{
				QueryFilter.Append(FString::Printf(TEXT("%s %s %s"), *SettingName, *ComparisonString, *SettingValue.ToString()));
			}
			else
			{
				QueryFilter.Append(FString::Printf(TEXT("%s %s '%s'"), *SettingName, *ComparisonString, *SettingValue.ToString()));
			}
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("ComposeLobbySearchQueryFilter: Unhandled search parameter. Add %s to the s_SearchKeyMappingTable to filter the lobby searching"), *SettingName);
		}
	}

	return QueryFilter;
}

void FPlayFabLobby::OnGetPlayFabIDsFromPlatformIDsCompleted(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, FPendingSendInviteData PendingSendInvite)
{
	UE_LOG_ONLINE(Verbose, TEXT("FPlayFabLobby::OnGetPlayFabIDsFromPlatformIDsCompleted bSucceeded: %u"), bSucceeded);

	TMap<FString, FString> PlatformIdToPlayFabIdMapping;
	if (!ParsePlayFabIdsFromPlatformIdsResponse(HttpResponse, PlatformIdToPlayFabIdMapping))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::OnGetPlayFabIDsFromPlatformIDsCompleted ParseTitlePlayersFromPlatformIDsResponse failed"));
		return;
	}

	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (!IdentityIntPtr.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::OnGetPlayFabIDsFromPlatformIDsCompleted Identity Interface is invalid"));
		return;
	}

	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	TSharedPtr<FPlayFabUser> LocalUser = PendingSendInvite.PlayFabSendingUser;
	if (LocalUser == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::OnGetPlayFabIDsFromPlatformIDsCompleted was called with a NULL PlayFabSendingUser"));
		return;
	}

	auto OnGetPlayerCombinedInfoRequestCompleted = TDelegate<void(FHttpRequestPtr, FHttpResponsePtr, bool)>::CreateLambda([this, PendingSendInvite](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		if (HttpResponse == nullptr || HttpResponse->GetResponseCode() != 200)
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::GetPlayerCombinedInfo returned NOT successful!"));
			return;
		}
		FString TitlePlayerAccount;
		if (!ParseTitlePlayerAccountFromPlayerCombinedInfoResponse(HttpResponse, TitlePlayerAccount))
		{
			UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::GetPlayerCombinedInfo failed to parse TitlePlayerAccount response!"));
			return;
		}
		InviteTitleAccountIDsToLobby(PendingSendInvite, TitlePlayerAccount);
	});

	TArray<TPair<FString, FString>> ExtraHeaders;
	ExtraHeaders.Add(MakeTuple(FString("X-Authorization"), LocalUser->GetSessionTicket()));

	for (const auto& PlatformIdToPlayFabId : PlatformIdToPlayFabIdMapping)
	{
		auto InfoRequestParameters = MakeShared<FJsonObject>();
		InfoRequestParameters->SetBoolField(TEXT("GetUserAccountInfo"), true);
		TSharedPtr<FJsonObject> HttpRequestJson = MakeShareable(new FJsonObject());
		HttpRequestJson->SetObjectField(TEXT("InfoRequestParameters"), InfoRequestParameters);
		const FString PlayFabId = PlatformIdToPlayFabId.Value;
		HttpRequestJson->SetStringField(TEXT("PlayFabId"), PlayFabId);
		FString RequestBody = SerializeRequestJson(HttpRequestJson);
		if (!MakePlayFabRestRequest(OSSPlayFab->GetAppId(), LocalUser->GetEntityToken(), TEXT("/Client/GetPlayerCombinedInfo"), ExtraHeaders, RequestBody, OnGetPlayerCombinedInfoRequestCompleted))
		{
			UE_LOG_ONLINE(Error, TEXT("MakePlayFabRestRequest for GetPlayerCombinedInfo failed!"));
		}
	}
}

void FPlayFabLobby::OnGetTitleAccountIDsFromPlatformIDsCompleted(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded, FPendingSendInviteData PendingSendInvite)
{
	UE_LOG_ONLINE(Verbose, TEXT("FPlayFabLobby::OnGetTitleAccountIDsFromPlatformIDsCompleted bSucceeded: %u"), bSucceeded);

	TMap<FString, FString> PlatformIdToTitleAccountIdMapping;
	if (!ParseTitleAccountIDsFromPlatformIDsResponse(HttpResponse, PlatformIdToTitleAccountIdMapping))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::OnGetTitleAccountIDsFromPlatformIDsCompleted ParseTitlePlayersFromPlatformIDsResponse failed"));
		return;
	}
	
	for (const auto& PlatformIdToTitleAccountIdKVP : PlatformIdToTitleAccountIdMapping)
	{
		InviteTitleAccountIDsToLobby(PendingSendInvite, PlatformIdToTitleAccountIdKVP.Value);
	}
}

void  FPlayFabLobby::InviteTitleAccountIDsToLobby(FPendingSendInviteData PendingSendInvite, const FString& TitleAccountID)
{
	PFLobbyHandle LobbyHandle = nullptr;
	if (!GetLobbyFromSession(PendingSendInvite.SessionName, LobbyHandle))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::InviteTitleAccountIDsToLobby: No lobby found for session %s!"), *(PendingSendInvite.SessionName.ToString()));
		return;
	}

	const PFEntityKey SenderEntityKey = PendingSendInvite.PlayFabSendingUser->GetEntityKey();

	std::string EntityIdStr(TCHAR_TO_UTF8(*TitleAccountID));
	std::string EntityTypeStr(TCHAR_TO_UTF8(TEXT(ENTITY_TYPE_TITLE_PLAYER)));

	PFEntityKey FriendsEntityKey = { EntityIdStr.c_str(), EntityTypeStr.c_str() };

	HRESULT Hr = PFLobbySendInvite(LobbyHandle, &SenderEntityKey, &FriendsEntityKey, nullptr);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE(Error, TEXT("FPlayFabLobby::PFLobbySendInvite failed: 0x%08x"), Hr);
	}
}

void FPlayFabLobby::BuildSearchKeyMappingTable()
{
	int32 SizeOfMappingTable = sizeof(s_SearchKeyMappingTable) / sizeof(struct FSearchKeyMappingTable);
	for (int32 i = 0; i < SizeOfMappingTable; ++ i)
	{
		FString SearchKey;
		switch (s_SearchKeyMappingTable[i].Type)
		{
			case EOnlineKeyValuePairDataType::Bool:
			case EOnlineKeyValuePairDataType::Int32:
				SearchKey = FString::Printf(TEXT("%skey%d"), *SEARCH_KEY_PREFIX_NUMBER, s_SearchKeyMappingTable[i].KeyNumber);
				break;
			case EOnlineKeyValuePairDataType::String:
				SearchKey = FString::Printf(TEXT("%skey%d"), *SEARCH_KEY_PREFIX_STRING, s_SearchKeyMappingTable[i].KeyNumber);
				break;
		}
		SearchKeyMappingTable.Add(SearchKey, TPair<FString, EOnlineKeyValuePairDataType::Type>(s_SearchKeyMappingTable[i].SettingKey.ToString(), s_SearchKeyMappingTable[i].Type));
	}
}

bool FPlayFabLobby::GetSearchKeyFromSettingMappingTable(const FString& SettingKey, FString& SearchKey, EOnlineKeyValuePairDataType::Type& Type) const
{
	for (const auto& SearchFilter : SearchKeyMappingTable)
	{
		if (SearchFilter.Value.Key.Equals(SettingKey, ESearchCase::IgnoreCase))
		{
			SearchKey = SearchFilter.Key;
			Type = SearchFilter.Value.Value;
			return true;
		}
	}
	return false;
}

TPair<FString, EOnlineKeyValuePairDataType::Type>* FPlayFabLobby::FindSearchKey(FString& SearchKey)
{
	return SearchKeyMappingTable.Find(SearchKey);
}
