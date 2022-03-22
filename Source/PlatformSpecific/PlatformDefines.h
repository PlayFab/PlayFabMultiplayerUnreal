//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

__pragma(warning(push))
__pragma(warning(disable: 4668)) /* 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives' */

#if OSS_PLAYFAB_WINGDK || OSS_PLAYFAB_XSX || OSS_PLAYFAB_XBOXONEGDK
#include "OnlineSubsystemGDKTypes.h"

typedef FUniqueNetIdGDK PLATFORM_UNIQUE_NET_ID;
typedef FUniqueNetIdGDKRef PLATFORM_UNIQUE_NET_ID_REF;
#endif

#if OSS_PLAYFAB_XBOXONE
#include "OnlineSubsystemLiveTypes.h"

typedef FUniqueNetIdLive PLATFORM_UNIQUE_NET_ID;
typedef FUniqueNetIdLiveRef PLATFORM_UNIQUE_NET_ID_REF;
#endif