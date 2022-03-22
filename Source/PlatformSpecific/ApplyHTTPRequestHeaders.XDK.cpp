//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HAL/Platform.h"
PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS

#if OSS_PLAYFAB_XBOXONE
#include "OnlineIdentityInterfacePlayFab.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"

// XDK includes
#include <Windows.foundation.h>
#include "OnlineSubsystemLiveTypes.h"
#include "OnlineIdentityInterfaceLive.h"
#include "XboxCommonAllowPlatformTypes.h"
#define _UITHREADCTXT_SUPPORT   0
#include <ppltasks.h>
#include "XboxCommonHidePlatformTypes.h"

using Windows::Xbox::System::GetTokenAndSignatureResult;
using Windows::Xbox::System::User;

void FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(bool bWasSuccessful, const FString& PlatformUserID, TMap<FString, FString> PlatformHeaders/* = TMap<FString, FString>()*/, TSharedPtr<FJsonObject> RequestBodyJson/* = TSharedPtr<FJsonObject>(nullptr)*/)
{
	IOnlineSubsystem* OSSPlayFab = IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM);
	if (OSSPlayFab)
	{
		FOnlineIdentityPlayFabPtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityPlayFab>(OSSPlayFab->GetIdentityInterface());
		if (IdentityInterface.IsValid())
		{
			IdentityInterface->FinishRequest(bWasSuccessful, PlatformUserID, PlatformHeaders, RequestBodyJson);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted: IdentityInterface is null"));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted: OSSPlayFab is null"));
	}
}

// NOTE: All error cases must call FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted so the request is handled and cleaned up in the shared OSS PF identity interface
bool FOnlineIdentityPlayFab::ApplyPlatformHTTPRequestData(const FString& PlatformUserID, const FString& URL, const FString& RequestVerb)
{
	// If we are in here, it's safe to assume the base OSS is XDK/Xbox live.
	FUniqueNetIdLiveRef LiveUser = FUniqueNetIdLive::Create(PlatformUserID);
	IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();
	if (NativeSubsystem)
	{
		IOnlineIdentityPtr NativeIdentityInterface = NativeSubsystem->GetIdentityInterface();
		if (NativeIdentityInterface && NativeIdentityInterface.IsValid())
		{
			FOnlineIdentityLive* IdentityInterfaceLive = static_cast<FOnlineIdentityLive*>(NativeIdentityInterface.Get());
			Windows::Xbox::System::User^ PlatformUser = IdentityInterfaceLive->GetUserForUniqueNetId(LiveUser.Get());
			if (PlatformUser)
			{
				try
				{
					Windows::Foundation::IAsyncOperation<GetTokenAndSignatureResult^>^ GetTokenAndSignatureOp = PlatformUser->GetTokenAndSignatureAsync(
						ref new Platform::String(*RequestVerb),
						ref new Platform::String(*URL),
						ref new Platform::String() // no headers
					);

					Concurrency::create_task(GetTokenAndSignatureOp).then([this, PlatformUserID](Concurrency::task<GetTokenAndSignatureResult^> t)
						{
							try
							{
								GetTokenAndSignatureResult^ Result = t.get();

								Platform::String^ TokenRaw = Result->Token;
								TMap<FString, FString> PlatformHeaders; // no header on xbox
								TSharedPtr<FJsonObject> RequestBodyJson = MakeShareable(new FJsonObject());
								RequestBodyJson->SetStringField(TEXT("XboxToken"), TokenRaw->Data());

								FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(true, PlatformUserID, PlatformHeaders, RequestBodyJson);
							}
							catch (Platform::Exception^ ex)
							{
								UE_LOG_ONLINE(Warning, TEXT("FOnlineIdentityPlayFab::ApplyPlatformHTTPRequestData: Failed with 0x%0.8X"), ex->HResult);
								FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(false, PlatformUserID);
							}
						});

					return true;
				}
				catch (Platform::COMException^ Ex)
				{
					UE_LOG_ONLINE(Warning, TEXT("FOnlineIdentityPlayFab::ApplyPlatformHTTPRequestData: Failed to get token. HResult = 0x%0.8X"), Ex->HResult);
					FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(false, PlatformUserID);
				}
			}
		}
	}

	return false;
}
#endif