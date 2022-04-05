//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HAL/Platform.h"
PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
#include "GDKUserManager.h"
#include "GDKTaskQueueHelpers.h"

#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include <xutility>
#include <XUser.h>
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"

#include "OnlineIdentityInterfacePlayFab.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"

void FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(bool bWasSuccessful, const FString& PlatformUserID, TMap<FString, FString> PlatformHeaders/* = TMap<FString, FString>()*/, TSharedPtr<FJsonObject> RequestBodyJson/* = TSharedPtr<FJsonObject>(nullptr)*/)
{
	IOnlineSubsystem* OSSPlayFab = IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM);
	if (OSSPlayFab)
	{
		FOnlineIdentityPlayFabPtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityPlayFab>(OSSPlayFab->GetIdentityInterface());
		if (IdentityInterface.IsValid())
		{
			RequestBodyJson->TryGetStringField(TEXT("XboxToken"), IdentityInterface->LocalUserXToken);
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
	XAsyncBlock* pNewAsyncBlock = new XAsyncBlock();
	int64 xuid = FCString::Atoi64(*PlatformUserID);

	ZeroMemory(pNewAsyncBlock, sizeof(*pNewAsyncBlock));
	pNewAsyncBlock->queue = FGDKAsyncTaskQueue::GetGenericQueue();
	pNewAsyncBlock->context = (void*)xuid;

	pNewAsyncBlock->callback = [](XAsyncBlock* ab)
	{
		int64 innerXuid = (int64)ab->context;
		const FString InnerPlatformUserID = FString::Printf(TEXT("%llu"), innerXuid);
		IOnlineSubsystem* OSSPlayFab = IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM);
		if (OSSPlayFab)
		{
			// Get the token
			TArray<uint8> BufferArray;
			bool bWasSuccessful = false;
			FString ResultToken;
			FString ResultSignature;
			uint64 ResultSize = 0;
			HRESULT Result = XUserGetTokenAndSignatureUtf16ResultSize(ab, &ResultSize);
			if (Result == S_OK)
			{
				BufferArray.Reserve(ResultSize);
				XUserGetTokenAndSignatureUtf16Data* ResultData = nullptr;

				Result = XUserGetTokenAndSignatureUtf16Result(
					ab,
					ResultSize,
					BufferArray.GetData(),
					&ResultData,
					nullptr);

				TMap<FString, FString> PlatformHeaders; // No header on xbox
				TSharedPtr<FJsonObject> RequestBodyJson = MakeShareable(new FJsonObject());

				if (Result == S_OK)
				{
					ResultToken = ResultData->token;
					bWasSuccessful = true;

					// Set token in JSON body
					RequestBodyJson->SetStringField(TEXT("XboxToken"), ResultToken);
				}
				else
				{
					UE_LOG_ONLINE(Error, TEXT("GetTokenAndSignatureResult failed with 0x%0.8X"), Result);
					bWasSuccessful = false;
				}

				FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(bWasSuccessful, InnerPlatformUserID, PlatformHeaders, RequestBodyJson);
			}
			else
			{
				UE_LOG_ONLINE(Error, TEXT("GetTokenAndSignatureResultSize failed with 0x%0.8X"), Result);
				bWasSuccessful = false;
				FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(false, InnerPlatformUserID);
			}
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("OnlineSubsystemPlayFabInstance is nullptr"));
			FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(false, InnerPlatformUserID);
		}

		delete ab;
	};

	auto& GDKUserMgr = FGDKUserManager::Get();
	const FGDKUserHandle userHandle = GDKUserMgr.GetUserHandleByXUserId(xuid);
	HRESULT Result = XUserGetTokenAndSignatureUtf16Async(userHandle, XUserGetTokenAndSignatureOptions::None, L"POST", L"https://playfabapi.com/", 0, nullptr, 0, nullptr, pNewAsyncBlock);

	if (Result != S_OK)
	{
		UE_LOG_ONLINE(Error, TEXT("[FOnlineIdentityPlayFab::AuthenticateUser] starting XUserGetTokenAndSignatureUtf16Async operation failed with code 0x%0.8X."), Result);
	}

	return Result == S_OK;
}
#endif