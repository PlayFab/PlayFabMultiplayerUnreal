//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#if defined(OSS_PLAYFAB_WIN64)
#include "OnlineExternalUIInterfacePlayFab.h"
#include "OnlineSessionInterfacePlayFab.h"

#define OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();  IOnlineExternalUIPtr NativeExternalUIInterface = NativeSubsystem ? NativeSubsystem->GetExternalUIInterface() : nullptr; if (NativeExternalUIInterface)

bool FOnlineExternalUIPlayFab::ShowInviteUI(int32 InLocalUserNum, FName InSessionName)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowInviteUI()"));
	
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		FOnlineSessionPlayFabPtr SessionIntPlayFab = OSSPlayFab ? OSSPlayFab->GetSessionInterfacePlayFab() : nullptr;
		if (SessionIntPlayFab.IsValid())
		{
			FName NativeSessionName = SessionIntPlayFab->GetNativeSessionName();
			return NativeExternalUIInterface->ShowInviteUI(InLocalUserNum, NativeSessionName);
		}
		else
		{
			UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("FOnlineExternalUIPlayFab::ShowInviteUI: Invalid online session interface."));
		}
	}

	return false;
}
#endif // OSS_PLAYFAB_WIN64