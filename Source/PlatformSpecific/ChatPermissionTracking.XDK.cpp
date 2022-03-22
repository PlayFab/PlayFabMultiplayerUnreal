//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HAL/Platform.h"
PRAGMA_DISABLE_UNDEFINED_IDENTIFIER_WARNINGS

#if OSS_PLAYFAB_XBOXONE
#include "PlatformDefines.h"

#include "OnlineVoiceInterfacePlayFab.h"
#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"

#include <PartyImpl.h>
#include <PartyTypes.h>
#include "PartyXboxLive.h"
#include "PartyXboxLiveImpl.h"

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
				uint64_t Xuid = 0;
				err = tokenAndSignatureRequested->localChatUser->GetXboxUserId(&Xuid);
				if (PARTY_FAILED(err))
				{
					UE_LOG_ONLINE(Error, TEXT("GetXboxUserId failed: %hs"), GetXblErrorMessage(err));
					break;
				}

				FUniqueNetIdLiveRef LiveUser = FUniqueNetIdLive::Create(FString::Printf(TEXT("%llu"), Xuid));
				IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();
				if (NativeSubsystem != nullptr)
				{
					IOnlineIdentityPtr NativeIdentityInterface = NativeSubsystem->GetIdentityInterface();

					if (NativeIdentityInterface != nullptr && NativeIdentityInterface.IsValid())
					{
						FOnlineIdentityLive* IdentityInterfaceLive = static_cast<FOnlineIdentityLive*>(NativeIdentityInterface.Get());
						Windows::Xbox::System::User^ platformUser = IdentityInterfaceLive->GetUserForUniqueNetId(LiveUser.Get());

						if (platformUser != nullptr)
						{
							try
							{
								Platform::String^ AdditionalHeaders = ref new Platform::String(TEXT(""));

								for (uint32_t j = 0; j < tokenAndSignatureRequested->headerCount; j++)
								{
									AdditionalHeaders += tokenAndSignatureRequested->headers[j].name;
									AdditionalHeaders += TEXT(":");
									AdditionalHeaders += tokenAndSignatureRequested->headers[j].value;
									AdditionalHeaders += TEXT("\n");
								}

								Windows::Foundation::IAsyncOperation<GetTokenAndSignatureResult^>^ GetTokenAndSignatureOp;

								if (tokenAndSignatureRequested->allUsers)
								{
									GetTokenAndSignatureOp = platformUser->GetTokenAndSignatureForAllUsersAsync(
										ref new Platform::String(UTF8_TO_TCHAR(tokenAndSignatureRequested->method)),
										ref new Platform::String(UTF8_TO_TCHAR(tokenAndSignatureRequested->url)),
										AdditionalHeaders
									);
								}
								else
								{
									GetTokenAndSignatureOp = platformUser->GetTokenAndSignatureAsync(
										ref new Platform::String(UTF8_TO_TCHAR(tokenAndSignatureRequested->method)),
										ref new Platform::String(UTF8_TO_TCHAR(tokenAndSignatureRequested->url)),
										AdditionalHeaders
									);
								}

								Concurrency::create_task(GetTokenAndSignatureOp).then([this, CorrelationId=tokenAndSignatureRequested->correlationId](Concurrency::task<GetTokenAndSignatureResult^> t)
								{
									try
									{
										GetTokenAndSignatureResult^ Result = t.get();

										Platform::String^ TokenRaw = Result->Token;

										// Tell the party manager we're done
										PartyError err = PartyXblManager::GetSingleton().CompleteGetTokenAndSignatureRequest(
											CorrelationId,
											true,
											TCHAR_TO_UTF8(Result->Token->Data()),
											TCHAR_TO_UTF8(Result->Signature->Data())
										);

										if (PARTY_SUCCEEDED(err))
										{
											UE_LOG_ONLINE(Verbose, TEXT("CompleteGetTokenAndSignatureRequest succeeded.\n\tToken: %s\n\tSignature: %s"), Result->Token->Data(), Result->Signature->Data());
										}
										else
										{
											UE_LOG_ONLINE(Error, TEXT("CompleteGetTokenAndSignatureRequest failed: %hs"), GetXblErrorMessage(err));
										}
									}
									catch (Platform::Exception^ ex)
									{
										UE_LOG_ONLINE(Warning, TEXT("FOnlineIdentityPlayFab::ApplyPlatformHTTPRequestData failed with 0x%0.8X"), ex->HResult);
										PartyXblManager::GetSingleton().CompleteGetTokenAndSignatureRequest(CorrelationId, false, nullptr, nullptr);
									}
								});
							}
							catch (Platform::COMException^ Ex)
							{
								UE_LOG_ONLINE(Warning, TEXT("FOnlineIdentityPlayFab::ApplyPlatformHTTPRequestData: Failed to get token. HResult = 0x%0.8X"), Ex->HResult);
								PartyXblManager::GetSingleton().CompleteGetTokenAndSignatureRequest(tokenAndSignatureRequested->correlationId, false, nullptr, nullptr);
							}
						}
					}
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