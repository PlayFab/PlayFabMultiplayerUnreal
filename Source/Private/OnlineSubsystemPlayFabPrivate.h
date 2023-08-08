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
#define SETTING_PLATFORM_MODEL FString(TEXT("PlatformModel"))
#define SETTING_HOST_NICKNAME FName(TEXT("OWNERNICKNAME"))

#define PLATFORM_MODEL_WINGDK FString(TEXT("WINGDK"))
#define PLATFORM_MODEL_XSX FString(TEXT("XSX"))
#define PLATFORM_MODEL_XB1 FString(TEXT("XB1"))
#define PLATFORM_MODEL_WIN64 FString(TEXT("WIN64"))
#define PLATFORM_MODEL_SWITCH FString(TEXT("SWITCH"))
#define PLATFORM_MODEL_PS4 FString(TEXT("PS4"))
#define PLATFORM_MODEL_PS5 FString(TEXT("PS5"))
#define PLATFORM_MODEL_SONY FString(TEXT("SONY"))

#if defined(OSS_PLAYFAB_WINGDK)
#define PLATFORM_MODEL PLATFORM_MODEL_WINGDK
#elif defined(OSS_PLAYFAB_XSX)
#define PLATFORM_MODEL PLATFORM_MODEL_XSX
#elif defined(OSS_PLAYFAB_XB1)
#define PLATFORM_MODEL PLATFORM_MODEL_XB1
#elif defined(OSS_PLAYFAB_WIN64)
#define PLATFORM_MODEL PLATFORM_MODEL_WIN64
#elif defined(OSS_PLAYFAB_SWITCH)
#define PLATFORM_MODEL PLATFORM_MODEL_SWITCH
#elif defined(OSS_PLAYFAB_PS4)
#define PLATFORM_MODEL PLATFORM_MODEL_PS4
#elif defined(OSS_PLAYFAB_PS5)
#define PLATFORM_MODEL PLATFORM_MODEL_PS5
#else
// The platform model is required for check the bypass voice chat permission.
#error "Unknow platform"
#endif

// Lobby related search property keys
// There are already predefined keys defined in PlayFabLobby.cpp from s_SearchKeyMappingTable.
// FPlayFabLobby::BuildSearchKeyMappingTable() will construct the predefined keys.
// Please be careful to avoid overlapping existing keys.
#define SEARCH_KEY_PREFIX_STRING FString(TEXT("string_"))
#define SEARCH_KEY_PREFIX_NUMBER FString(TEXT("number_"))
#define SEARCH_KEY_PLATFORM_ID FString(TEXT("string_key1"))
#define SEARCH_KEY_HOST_NICKNAME FString(TEXT("string_key2"))
#define SEARCH_KEY_NATIVE_SESSIONID FString(TEXT("string_key3"))
#define SEARCH_KEY_NATIVE_PLATFORM FString(TEXT("string_key4"))

#define ENTITY_TYPE_TITLE_PLAYER "title_player_account"