//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#ifdef _MSC_VER
__pragma(warning(push))
__pragma(warning(disable: 4668)) /* 'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives' */
#endif // _MSC_VER

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
#include "OnlineSubsystemGDKTypes.h"

typedef FUniqueNetIdGDK PLATFORM_UNIQUE_NET_ID;
typedef FUniqueNetIdGDKRef PLATFORM_UNIQUE_NET_ID_REF;
#endif

#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_WIN64) || defined(OSS_PLAYFAB_PLAYSTATION)
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemPlayFabTypes.h"

typedef FUniqueNetIdPlayFab PLATFORM_UNIQUE_NET_ID;
typedef TSharedRef<const class FUniqueNetIdPlayFab> PLATFORM_UNIQUE_NET_ID_REF;
#endif