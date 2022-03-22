//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CoreTypes.h"

#define INVALID_INDEX -1

// URL Prefix when using PlayFab Party connection
#define PLAYFAB_URL_PREFIX TEXT("PlayFab.")

// Pre-pended to all PlayFab logging
#undef ONLINE_LOG_PREFIX
#define ONLINE_LOG_PREFIX TEXT("PLAYFAB: ")

#define SETTING_NETWORK_ID FName(TEXT("NETWORKID"))
#define SETTING_NETWORK_DESCRIPTOR FName(TEXT("NETWORKDESCRIPTOR"))
#define SETTING_HOST_CONNECT_INFO FName(TEXT("HOSTCONNECTINFO"))

// Match related strings
#define SETTING_MATCH_QUEUENAME FName(TEXT("MATCHMAKINGQUEUE"))
#define SETTING_MATCH_ATTRIBUTES_KEY FName(TEXT("USERATTRIBUTES"))

// Lobby related strings
#define SETTING_CONNECTION_STRING FName(TEXT("CONNECTIONSTRING"))
#define SETTING_PLATFORM_ID FString(TEXT("PlatformId"))

// Lobby related search property keys
#define SEARCH_KEY_PREFIX_STRING FString(TEXT("string_"))
#define SEARCH_KEY_PREFIX_NUMBER FString(TEXT("number_"))
#define SEARCH_KEY_PLATFORM_ID FString(TEXT("string_key1"))

#define ENTITY_TYPE_TITLE_PLAYER "title_player_account"