//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#if defined(OSS_PLAYFAB_WIN64)
#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"

#define OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();  IOnlineSessionPtr NativeSessionInterface = NativeSubsystem ? NativeSubsystem->GetSessionInterface() : nullptr; if (NativeSessionInterface)

bool FOnlineSessionPlayFab::SendInvite(const FUniqueNetId& SenderId, FName SessionName, const TArray< TSharedRef<const FUniqueNetId> >& RemoteUserNetIds)
{
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("FOnlineSessionPlayFab::SendInvite()"));

	OSS_PLAYFAB_GET_NATIVE_SESSION_INTERFACE
	{
		return NativeSessionInterface->SendSessionInviteToFriends(SenderId, NativeSessionName, RemoteUserNetIds);
	}
	return false;
}

#endif // OSS_PLAYFAB_WIN64