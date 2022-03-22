//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HAL/Platform.h"
PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS

#if OSS_PLAYFAB_WINGDK || OSS_PLAYFAB_XSX || OSS_PLAYFAB_XBOXONEGDK
#include "PlatformDefines.h"

#include "OnlineVoiceInterfacePlayFab.h"
#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"

#include <PartyImpl.h>
#include <PartyTypes.h>
#include "PartyXboxLive.h"
#include "PartyXboxLiveImpl.h"
#include <GDKUserManager.h>

using namespace Party;

namespace
{
	bool bPartyXblManagerInitialized = false;

	struct RemoteTalkerOptions
	{
		TMap<uint64_t, PartyChatPermissionOptions> OptionsMap;
	};

	TMap<uint64_t, PartyXblChatUser*> TalkersMap;
	TMap<uint64_t, RemoteTalkerOptions> TalkersOptions;
	TMap<FString, uint64_t> TalkerIdMapping;

	PartyXblLocalChatUser* LocalChatUser = nullptr;

	PartyString GetXblErrorMessage(PartyError error)
	{
		PartyString errString = nullptr;

		PartyError err = PartyXblManager::GetErrorMessage(error, &errString);
		if (PARTY_FAILED(err))
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to get error message %lu."), err);
			return "[ERROR]";
		}

		return errString;
	}
}

PartyChatPermissionOptions FOnlineVoicePlayFab::GetChatPermissionsForTalker(const FString& LocalId, const FString& RemoteId)
{
	RemoteTalkerOptions* AllOptions = nullptr;
	PartyChatPermissionOptions* Options = nullptr;

	UE_LOG_ONLINE(Verbose, TEXT("GetChatPermissionsForTalker %s vs. %s"), *LocalId, *RemoteId);

	AllOptions = TalkersOptions.Find(FCString::Atoi64(*LocalId));
	if (AllOptions != nullptr)
	{
		Options = (*AllOptions).OptionsMap.Find(FCString::Atoi64(*RemoteId));
		if (Options != nullptr)
		{
			return *Options;
		}
	}

	UE_LOG_ONLINE(Verbose, TEXT("No options found"));

	return PartyChatPermissionOptions::None;
}

FString FOnlineVoicePlayFab::GetPlatformIdFromEntityId(const FString& EntityId)
{
	UE_LOG_ONLINE(Verbose, TEXT("GetPlatformIdFromEntityId %s"), *EntityId);

	uint64_t* Xuid = TalkerIdMapping.Find(EntityId);
	if (Xuid != nullptr)
	{
		return FString::Printf(TEXT("%llu"), *Xuid);
	}

	UE_LOG_ONLINE(Error, TEXT("Failed to find xuid mapping for %s"), *EntityId);

	return FString();
}

void FOnlineVoicePlayFab::StartTrackingPermissionForTalker(const FString& UserId, bool IsRemote)
{
	UE_LOG_ONLINE(Verbose, TEXT("StartTrackingPermissionForTalker %s %s"), *UserId, IsRemote ? TEXT("remote") : TEXT("local"));

	if (bPartyXblManagerInitialized == false)
	{
		FString TitleID;
		GConfig->GetString(TEXT("OnlineSubsystemPlayFab"), TEXT("PlayFabTitleID"), TitleID, GEngineIni);

		PartyError err = PartyXblManager::GetSingleton().Initialize(TCHAR_TO_UTF8(*TitleID));
		if (PARTY_FAILED(err))
		{
			UE_LOG_ONLINE(Error, TEXT("Party XboxLive Initialize failed: %hs"), GetXblErrorMessage(err));
			return;
		}

		bPartyXblManagerInitialized = true;
	}

	uint64_t XboxId = FCString::Atoi64(*UserId);

	if (IsRemote)
	{
		PartyXblChatUser* NewUser = nullptr;
		PartyError err = PartyXblManager::GetSingleton().CreateRemoteChatUser(XboxId, &NewUser);
		if (PARTY_SUCCEEDED(err))
		{
			TalkersMap.Emplace(XboxId, NewUser);
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("CreateRemoteChatUser failed: %hs"), GetXblErrorMessage(err));
		}
	}
	else
	{
		PartyXblLocalChatUser* NewUser = nullptr;
		PartyError err = PartyXblManager::GetSingleton().CreateLocalChatUser(XboxId, nullptr, &NewUser);
		if (PARTY_SUCCEEDED(err))
		{
			TalkersMap.Emplace(XboxId, NewUser);
			LocalChatUser = NewUser;
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("CreateLocalChatUser failed: %hs"), GetXblErrorMessage(err));
		}
	}

	TalkersOptions.Emplace(XboxId, RemoteTalkerOptions());

	PartyError err = PartyXblManager::GetSingleton().GetEntityIdsFromXboxLiveUserIds(1, &XboxId, LocalChatUser, nullptr);
	if (PARTY_FAILED(err))
	{
		UE_LOG_ONLINE(Error, TEXT("GetEntityIdsFromXboxLiveUserIds failed: %hs"), GetXblErrorMessage(err));
	}
}

void FOnlineVoicePlayFab::StopTrackingPermissionForTalker(const FString& UserId)
{
	UE_LOG_ONLINE(Verbose, TEXT("StopTrackingPermissionForTalker %s"), *UserId);

	PartyXblChatUser** User;
	uint64_t xuid = FCString::Atoi64(*UserId);

	User = TalkersMap.Find(xuid);
	if (User != nullptr)
	{
		PartyError err = PartyXblManager::GetSingleton().DestroyChatUser(*User);
		if (PARTY_FAILED(err))
		{
			UE_LOG_ONLINE(Error, TEXT("DestroyChatUser failed: %hs"), GetXblErrorMessage(err));
		}

		TalkersMap.Remove(xuid);
		TalkersOptions.Remove(xuid);

		for (auto& TalkerIdKvPair : TalkerIdMapping)
		{
			if (TalkerIdKvPair.Value == xuid)
			{
				TalkerIdMapping.Remove(TalkerIdKvPair.Key);
				break;
			}
		}
	}

	if (TalkersMap.Num() == 0)
	{
		PartyError err = PartyXblManager::GetSingleton().Cleanup();
		if (PARTY_FAILED(err))
		{
			UE_LOG_ONLINE(Error, TEXT("PartyXblManager::Cleanup() failed: %hs"), GetXblErrorMessage(err));
		}
	}
}

void FOnlineVoicePlayFab::TickTalkerPermissionTracking()
{
	if (bPartyXblManagerInitialized == false)
	{
		return;
	}
	
	if (TalkersMap.Num() == 0)
	{
		return;
	}

	uint32_t count;
	PartyXblStateChangeArray xblChanges;

	// Process Xbl messages
	PartyError err = PartyXblManager::GetSingleton().StartProcessingStateChanges(
		&count,
		&xblChanges
	);

	if (PARTY_FAILED(err))
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineVoicePlayFab::TickTalkerPermissionTracking: StartProcessingStateChanges failed: %hs"), GetXblErrorMessage(err));
		return;
	}

	for (uint32_t i = 0; i < count; i++)
	{
		auto change = xblChanges[i];

		switch (change->stateChangeType)
		{
		case PartyXblStateChangeType::CreateLocalChatUserCompleted:
			UE_LOG_ONLINE(Verbose, TEXT("PartyXblStateChangeType: CreateLocalChatUserCompleted"));
			break;

		case PartyXblStateChangeType::LocalChatUserDestroyed:
			UE_LOG_ONLINE(Verbose, TEXT("PartyXblStateChangeType: LocalChatUserDestroyed"));
			break;

		case PartyXblStateChangeType::LoginToPlayFabCompleted:
			UE_LOG_ONLINE(Verbose, TEXT("PartyXblStateChangeType: LoginToPlayFabCompleted"));
			break;

		case PartyXblStateChangeType::GetEntityIdsFromXboxLiveUserIdsCompleted:
		{
			UE_LOG_ONLINE(Verbose, TEXT("PartyXblStateChangeType: PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange"));
			auto userStateChange = static_cast<const PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange*>(change);

			if (userStateChange->result != PartyXblStateChangeResult::Succeeded)
			{
				check(userStateChange->result == PartyXblStateChangeResult::Succeeded);
				// This should not fail because users are required to be signed in to PlayFab with their XUID before
				// we attempt to get the EntityIds.
				break;
			}

			for (uint32_t j = 0; j < userStateChange->entityIdMappingCount; j++)
			{
				TalkerIdMapping.Add(userStateChange->entityIdMappings[j].playfabEntityId, userStateChange->entityIdMappings[j].xboxLiveUserId);
				UE_LOG_ONLINE(Verbose, TEXT("TalkerIdMapping: %llu to %hs"), userStateChange->entityIdMappings[j].xboxLiveUserId, userStateChange->entityIdMappings[j].playfabEntityId);
			}

			// update chat permissions for all, this can arrive after remote chat controls are created
			UpdatePermissionsForAllControls();

			break;
		}

		case PartyXblStateChangeType::RequiredChatPermissionInfoChanged:
		{
			UE_LOG_ONLINE(Verbose, TEXT("PartyXblStateChangeType: RequiredChatPermissionInfoChanged"));
			auto chatPermissionChanged = static_cast<const PartyXblRequiredChatPermissionInfoChangedStateChange*>(change);

			PartyXblLocalChatUser* localChatUser = chatPermissionChanged->localChatUser;
			PartyXblChatUser* targetChatUser = chatPermissionChanged->targetChatUser;

			PartyXblChatPermissionInfo chatPermissionInfo;
			err = localChatUser->GetRequiredChatPermissionInfo(targetChatUser, &chatPermissionInfo);
			if (PARTY_FAILED(err))
			{
				UE_LOG_ONLINE(Error, TEXT("GetRequiredChatPermissionInfo failed: %hs"), GetXblErrorMessage(err));
				break;
			}

			uint64_t userXuid = 0;
			err = localChatUser->GetXboxUserId(&userXuid);
			if (PARTY_FAILED(err))
			{
				UE_LOG_ONLINE(Error, TEXT("GetXboxUserId on local failed: %hs"), GetXblErrorMessage(err));
				break;
			}

			RemoteTalkerOptions* LocalTalker = TalkersOptions.Find(userXuid);
			if (LocalTalker == nullptr)
			{
				UE_LOG_ONLINE(Error, TEXT("Can't find TalkersOptions entry for %llu!"), userXuid);
				break;
			}

			uint64_t remoteXuid = 0;
			err = targetChatUser->GetXboxUserId(&remoteXuid);
			if (PARTY_FAILED(err))
			{
				UE_LOG_ONLINE(Error, TEXT("GetXboxUserId on remote failed: %hs"), GetXblErrorMessage(err));
				break;
			}

			(*LocalTalker).OptionsMap.Emplace(remoteXuid, chatPermissionInfo.chatPermissionMask);

			OnChatPermissionsChanged(FString::Printf(TEXT("%llu"), userXuid), FString::Printf(TEXT("%llu"), remoteXuid), chatPermissionInfo.chatPermissionMask);
			break;
		}

		case PartyXblStateChangeType::TokenAndSignatureRequested:
		{
			UE_LOG_ONLINE(Verbose, TEXT("PartyXblStateChangeType: TokenAndSignatureRequested"));

			auto tokenAndSignatureRequested = static_cast<const PartyXblTokenAndSignatureRequestedStateChange*>(change);

			// Setup the options flags
			XUserGetTokenAndSignatureOptions tnsOptions = tokenAndSignatureRequested->forceRefresh ?
				XUserGetTokenAndSignatureOptions::ForceRefresh : XUserGetTokenAndSignatureOptions::None;

			if (tokenAndSignatureRequested->allUsers)
			{
				tnsOptions |= XUserGetTokenAndSignatureOptions::AllUsers;
			}

			// Format the headers
			std::vector<XUserGetTokenAndSignatureHttpHeader> headers;
			for (uint32_t j = 0; j < tokenAndSignatureRequested->headerCount; j++)
			{
				headers.push_back({ tokenAndSignatureRequested->headers[j].name, tokenAndSignatureRequested->headers[j].value });
			}

			XAsyncBlock* async = new XAsyncBlock{};

			async->context = (void*)(UINT_PTR)tokenAndSignatureRequested->correlationId;
			async->callback = [](XAsyncBlock* async)
			{
				size_t bufferSize;
				HRESULT hr = XUserGetTokenAndSignatureResultSize(async, &bufferSize);

				if (FAILED(hr))
				{
					UE_LOG_ONLINE(Error, TEXT("XUserGetTokenAndSignatureResultSize failed: 0x%08X"), static_cast<unsigned int>(hr));
				}

				std::vector<uint8_t> buffer(bufferSize);
				XUserGetTokenAndSignatureData* data = nullptr;

				if (SUCCEEDED(hr))
				{
					hr = XUserGetTokenAndSignatureResult(async, buffer.size(), buffer.data(), &data, nullptr /*bufferUsed*/);

					if (FAILED(hr))
					{
						UE_LOG_ONLINE(Error, TEXT("XUserGetTokenAndSignatureResult failed: 0x%08X"), static_cast<unsigned int>(hr));
					}
				}

				// Tell the party manager we're done
				PartyError err = PartyXblManager::GetSingleton().CompleteGetTokenAndSignatureRequest(
					(UINT_PTR)(async->context),
					SUCCEEDED(hr),
					data ? data->token : nullptr,
					data ? data->signature : nullptr
				);

				if (PARTY_SUCCEEDED(err))
				{
					UE_LOG_ONLINE(Verbose, TEXT("CompleteGetTokenAndSignatureRequest succeeded.\n\tToken: %s\n\tSignature: %s"), data ? data->token : "nullptr", data ? data->signature : "nullptr");
				}
				else
				{
					UE_LOG_ONLINE(Error, TEXT("CompleteGetTokenAndSignatureRequest failed: %hs"), GetXblErrorMessage(err));
				}

				delete async;
			};

			uint64_t xuid = 0;
			err = tokenAndSignatureRequested->localChatUser->GetXboxUserId(&xuid);
			if (PARTY_FAILED(err))
			{
				UE_LOG_ONLINE(Error, TEXT("GetXboxUserId failed: %hs"), GetXblErrorMessage(err));
				break;
			}

			auto& GDKUserMgr = FGDKUserManager::Get();
			const FGDKUserHandle userHandle = GDKUserMgr.GetUserHandleByXUserId(xuid);

			// Start the auth call
			HRESULT hr = XUserGetTokenAndSignatureAsync(
				userHandle,                                             // XUser to query
				tnsOptions,                                             // Option flags
				tokenAndSignatureRequested->method,                     // HTTP method ("GET", "POST")
				tokenAndSignatureRequested->url,                        // Auth url
				static_cast<uint32_t>(headers.size()),                  // Additional header count
				headers.data(),                                         // Additional headers
				tokenAndSignatureRequested->bodySize,                   // Query body size
				tokenAndSignatureRequested->body,                       // Query body
				async
			);

			if (FAILED(hr))
			{
				UE_LOG_ONLINE(Error, TEXT("XUserGetTokenAndSignatureAsync failed: 0x%x"), hr);

				PartyXblManager::GetSingleton().CompleteGetTokenAndSignatureRequest(
					tokenAndSignatureRequested->correlationId,
					false,
					nullptr,
					nullptr
				);

				delete async;
			}
			break;
		}
		}
	}

	err = PartyXblManager::GetSingleton().FinishProcessingStateChanges(count, xblChanges);

	if (PARTY_FAILED(err))
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineVoicePlayFab::TickTalkerPermissionTracking: FinishProcessingStateChanges failed: %hs"), GetXblErrorMessage(err));
	}
}

void FOnlineVoicePlayFab::CleanUpPartyXblManager()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineVoicePlayFab::CleanUpPartyXblManager"));

	if (bPartyXblManagerInitialized)
	{
		PartyError err = PartyXblManager::GetSingleton().Cleanup();
		if (PARTY_FAILED(err))
		{
			UE_LOG_ONLINE(Error, TEXT("PartyXblManager::Cleanup() failed: %hs"), GetXblErrorMessage(err));
		}

		bPartyXblManagerInitialized = false;
	}
}

void FOnlineVoicePlayFab::InitPartyXblManager()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineVoicePlayFab::InitPartyXblManager"));

	if (bPartyXblManagerInitialized == false)
	{
		FString TitleID;
		GConfig->GetString(TEXT("OnlineSubsystemPlayFab"), TEXT("PlayFabTitleID"), TitleID, GEngineIni);

		PartyError err = PartyXblManager::GetSingleton().Initialize(TCHAR_TO_UTF8(*TitleID));
		if (PARTY_FAILED(err))
		{
			UE_LOG_ONLINE(Error, TEXT("Party XboxLive Initialize failed: %hs"), GetXblErrorMessage(err));
		}

		bPartyXblManagerInitialized = true;
	}
}
#endif