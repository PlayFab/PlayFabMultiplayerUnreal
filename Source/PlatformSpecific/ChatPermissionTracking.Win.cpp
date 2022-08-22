//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "HAL/Platform.h"
PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS

#if OSS_PLAYFAB_WIN64
#include "OnlineSessionSettings.h"
#include "PlatformDefines.h"

#include "OnlineVoiceInterfacePlayFab.h"
#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"

THIRD_PARTY_INCLUDES_START
#include <PartyImpl.h>
#include <PartyTypes.h>
THIRD_PARTY_INCLUDES_END

using namespace Party;

namespace
{
	struct RemoteTalkerOptions
	{
		ECrossNetworkType Type = ECrossNetworkType::DISABLED;
		TMap<FString /*User ID*/, PartyChatPermissionOptions> OptionsMap;
	};

	TMap<FString /*User ID*/, RemoteTalkerOptions> TalkersOptions;
	TMap<FString /*Entity ID*/, FString /*User ID*/> TalkerIdMapping;

	FString LocalTalkerUserId;

	TArray<FCrossNetworkTalkerPlayFab> CrossNetworkTalkers;
	
	bool UpdateTalkerCrossNetworkPermission()
	{
		if (CrossNetworkTalkers.Num() == 0)
		{
			return false;
		}

		if (LocalTalkerUserId.IsEmpty())
		{
			UE_LOG_ONLINE(Verbose, TEXT("Local user id isn't set yet"));
			return false;
		}

		auto CrossNetwork = CrossNetworkTalkers[0];

		Party::PartyChatPermissionOptions ChatPermissionMask = PartyChatPermissionOptions::None;
		RemoteTalkerOptions* LocalTalker = TalkersOptions.Find(LocalTalkerUserId);
		if (LocalTalker)
		{
			Party::PartyChatPermissionOptions* PermissionOptions = LocalTalker->OptionsMap.Find(CrossNetwork.RemoteUserId);
			if (PermissionOptions)
			{
				ChatPermissionMask = *PermissionOptions;
			}
		}
		else
		{
			TalkersOptions.Emplace(LocalTalkerUserId, RemoteTalkerOptions());
			LocalTalker = TalkersOptions.Find(LocalTalkerUserId);
		}

		if (CrossNetwork.Type != ECrossNetworkType::DISABLED)
		{
			ChatPermissionMask = Party::PartyChatPermissionOptions::SendAudio | Party::PartyChatPermissionOptions::ReceiveAudio | Party::PartyChatPermissionOptions::ReceiveText;
		}

		LocalTalker->Type = CrossNetwork.Type;
		LocalTalker->OptionsMap.Emplace(CrossNetwork.RemoteUserId, ChatPermissionMask);

		UE_LOG_ONLINE(Verbose, TEXT("Set voice chat permission %s vs %s as %d, Type %d"), *LocalTalkerUserId, *CrossNetwork.RemoteUserId, ChatPermissionMask, CrossNetwork.Type);

		CrossNetworkTalkers.RemoveAt(0, 1);
		return true;
	}
}

void FOnlineVoicePlayFab::AddTalkerIdMapping(const FString& EntityId, const FString& UserId)
{
	TalkerIdMapping.Add(EntityId, UserId);
	UE_LOG_ONLINE(Verbose, TEXT("Talker ID Mapping %s to %s"), *UserId, *EntityId);
}

void FOnlineVoicePlayFab::SetTalkerCrossNetworkPermission(ECrossNetworkType VoiceChatType, const FString& RemoteUserId, const FString& PlatformModel)
{
	FCrossNetworkTalkerPlayFab CrossNetwork = {VoiceChatType, RemoteUserId, PlatformModel};
	CrossNetworkTalkers.Emplace(CrossNetwork);
}

PartyChatPermissionOptions FOnlineVoicePlayFab::GetChatPermissionsForTalker(const FString& LocalId, const FString& RemoteId)
{
	RemoteTalkerOptions* AllOptions = nullptr;
	PartyChatPermissionOptions* Options = nullptr;

	UE_LOG_ONLINE(Verbose, TEXT("GetChatPermissionsForTalker %s vs. %s"), *LocalId, *RemoteId);

	AllOptions = TalkersOptions.Find(LocalId);
	if (AllOptions != nullptr)
	{
		Options = (*AllOptions).OptionsMap.Find(RemoteId);
		if (Options != nullptr)
		{
			return *Options;
		}
	}

	UE_LOG_ONLINE(Verbose, TEXT("No options found"));

	return PartyChatPermissionOptions::None;
}

FString FOnlineVoicePlayFab::GetPlatformIdFromEntityId(const FString& EntityId)
{
	UE_LOG_ONLINE(Verbose, TEXT("GetPlatformIdFromEntityId %s"), *EntityId);

	FString* Xuid = TalkerIdMapping.Find(EntityId);
	if (Xuid != nullptr)
	{
		return *Xuid;
	}

	UE_LOG_ONLINE(Error, TEXT("Failed to find xuid mapping for %s"), *EntityId);

	return FString();
}

void FOnlineVoicePlayFab::StartTrackingPermissionForTalker(const FString& UserId, bool IsRemote)
{
	UE_LOG_ONLINE(Verbose, TEXT("StartTrackingPermissionForTalker %s %s"), *UserId, IsRemote ? TEXT("remote") : TEXT("local"));

	if (!IsRemote)
	{
		LocalTalkerUserId = UserId;
	}
}

void FOnlineVoicePlayFab::StopTrackingPermissionForTalker(const FString& UserId)
{
	UE_LOG_ONLINE(Verbose, TEXT("StopTrackingPermissionForTalker %s"), *UserId);

	for (auto& TalkerIdKvPair : TalkerIdMapping)
	{
		if (TalkerIdKvPair.Value == UserId)
		{
			TalkerIdMapping.Remove(TalkerIdKvPair.Key);
			break;
		}
	}
}

void FOnlineVoicePlayFab::TickTalkerPermissionTracking()
{
	if (UpdateTalkerCrossNetworkPermission())
	{
		UpdatePermissionsForAllControls();
	}
}

void FOnlineVoicePlayFab::CleanUpPartyXblManager()
{
}

void FOnlineVoicePlayFab::InitPartyXblManager()
{
}
#endif