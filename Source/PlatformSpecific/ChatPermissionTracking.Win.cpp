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

PartyChatPermissionOptions FOnlineVoicePlayFab::GetChatPermissionsForTalker(const FString& LocalId, const FString& RemoteId)
{
	return PartyChatPermissionOptions::None;
}

FString FOnlineVoicePlayFab::GetPlatformIdFromEntityId(const FString& EntityId)
{
	return FString();
}

void FOnlineVoicePlayFab::StartTrackingPermissionForTalker(const FString& UserId, bool IsRemote)
{
}

void FOnlineVoicePlayFab::StopTrackingPermissionForTalker(const FString& UserId)
{
}

void FOnlineVoicePlayFab::TickTalkerPermissionTracking()
{
}

void FOnlineVoicePlayFab::CleanUpPartyXblManager()
{
}

void FOnlineVoicePlayFab::InitPartyXblManager()
{
}
#endif