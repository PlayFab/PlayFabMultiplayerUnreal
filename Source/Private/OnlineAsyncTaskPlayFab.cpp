//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifdef OSS_PLAYFAB_PLAYSTATION
#include "OnlineAsyncTaskPlayFab.h"

#include "EngineLogs.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"

void FOnlineAsyncTaskManagerPlayFab::OnlineTick()
{
	check(OSSPlayFab);
	check(IsInOnlineThread());
}

bool FOnlineAsyncTaskManagerPlayFab::IsInOnlineThread() const
{
	return FPlatformTLS::GetCurrentThreadId() == OnlineThreadId;
}
#endif // OSS_PLAYFAB_PLAYSTATION
