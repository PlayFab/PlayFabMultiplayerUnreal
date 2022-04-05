//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "OnlineCognitiveServicesInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"

FOnlineCognitiveServicesPlayFab::FOnlineCognitiveServicesPlayFab(class FOnlineSubsystemPlayFab* InSubsystem) 
	: OSSPlayFab(InSubsystem)
{
}

void FOnlineCognitiveServicesPlayFab::SetTextChatTranslationOptions(const FUniqueNetId& UserId, bool bTranslateToLocalLanguage)
{
	if (UserId.IsValid() == false)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextChatTranslationOptions: UserId was invalid"));
		return;
	}
	
	PartyLocalChatControl* LocalChatControl = GetLocalChatControl(UserId);
	if (LocalChatControl == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextChatTranslationOptions: Could not get LocalChatControl for User:%s"), *UserId.ToString());
		return;
	}

	PartyTextChatOptions Options = bTranslateToLocalLanguage ? PartyTextChatOptions::TranslateToLocalLanguage : PartyTextChatOptions::None;

	PartyError Err = LocalChatControl->SetTextChatOptions(Options, nullptr);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextChatTranslationOptions failed, %s, User:%s"), *GetPartyErrorMessage(Err), *UserId.ToString());
		return;
	}
}

void FOnlineCognitiveServicesPlayFab::SetVoiceChatTranscriptionOptions(const FUniqueNetId& UserId, bool bTranscribeSelf, bool bTranscribeOtherChatControlsWithMatchingLanguages, bool bTranscribeOtherChatControlsWithNonMatchingLanguages, bool bTranslateToLocalLanguage)
{
	if (UserId.IsValid() == false)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetVoiceChatTranscriptionOptions: UserId was invalid"));
		return;
	}
	
	PartyLocalChatControl* LocalChatControl = GetLocalChatControl(UserId);
	if (LocalChatControl == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextChatTranslationOptions: Could not get LocalChatControl for User:%s"), *UserId.ToString());
		return;
	}

	PartyVoiceChatTranscriptionOptions Options = PartyVoiceChatTranscriptionOptions::None;

	if (bTranscribeSelf)
	{
		Options |= PartyVoiceChatTranscriptionOptions::TranscribeSelf;
	}

	if (bTranscribeOtherChatControlsWithMatchingLanguages)
	{
		Options |= PartyVoiceChatTranscriptionOptions::TranscribeOtherChatControlsWithMatchingLanguages;
	}

	if (bTranscribeOtherChatControlsWithNonMatchingLanguages)
	{
		Options |= PartyVoiceChatTranscriptionOptions::TranscribeOtherChatControlsWithNonMatchingLanguages;
	}

	if (bTranslateToLocalLanguage)
	{
		Options |= PartyVoiceChatTranscriptionOptions::TranslateToLocalLanguage;
	}

	PartyError Err = LocalChatControl->SetTranscriptionOptions(Options, nullptr);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextChatTranslationOptions failed, %s, User:%s"), *GetPartyErrorMessage(Err), *UserId.ToString());
		return;
	}
}

void FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions(const FUniqueNetId& UserId, ETextToSpeechType TextToSpeechType)
{
	if (UserId.IsValid() == false)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions: UserId was invalid"));
		return;
	}
	
	PartyLocalChatControl* LocalChatControl = GetLocalChatControl(UserId);
	if (LocalChatControl == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions: Could not get LocalChatControl for User:%s"), *UserId.ToString());
		return;
	}

	// Get our current language
	PartyString LanguageCode = nullptr;
	PartyError Err = LocalChatControl->GetLanguage(&LanguageCode);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions: GetLanguage for LocalChatControl failed:, %s"), *GetPartyErrorMessage(Err));
		return;
	}

	FString CurrentLanguageCode = ANSI_TO_TCHAR(LanguageCode);
	PartySynthesizeTextToSpeechType PartyTTSType = TextToSpeechType == ETextToSpeechType::VoiceChat ? PartySynthesizeTextToSpeechType::VoiceChat : PartySynthesizeTextToSpeechType::Narration;

	// Get the profile list
	uint32 ProfileCount = 0;
	PartyTextToSpeechProfileArray ProfileList = nullptr;
	Err = LocalChatControl->GetAvailableTextToSpeechProfiles(&ProfileCount, &ProfileList);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions failed, %s, User:%s"), *GetPartyErrorMessage(Err), *UserId.ToString());
		return;
	}

	if (ProfileCount == 0)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions failed, PartyTextToSpeechProfileArray was empty"));
		return;
	}

	// Loop the profiles looking for one that matches our language
	for (uint32 i = 0; i < ProfileCount; ++i)
	{
		PartyTextToSpeechProfile* TTSProfile = ProfileList[i];
		if (TTSProfile)
		{
			PartyString ProfileLanguageCode = nullptr;
			Err = TTSProfile->GetLanguageCode(&ProfileLanguageCode);
			if (PARTY_FAILED(Err))
			{
				UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions: GetLanguageCode for TTSProfile failed:, %s"), *GetPartyErrorMessage(Err));
				continue;
			}

			if (CurrentLanguageCode == ANSI_TO_TCHAR(ProfileLanguageCode))
			{
				// Get the profile name
				PartyString ProfileName = nullptr;
				Err = TTSProfile->GetIdentifier(&ProfileName);
				if (PARTY_FAILED(Err))
				{
					UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions: GetIdentifier for TTSProfile failed:, %s"), *GetPartyErrorMessage(Err));
					return;
				}

				Err = LocalChatControl->SetTextToSpeechProfile(PartyTTSType, ProfileName, nullptr);
				if (PARTY_FAILED(Err))
				{
					UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions: SetTextToSpeechProfile failed:, %s"), *GetPartyErrorMessage(Err));
				}

				UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::SetTextToSpeechOptions: Setting TTS profile to %s"), ANSI_TO_TCHAR(ProfileName));

				return;
			}
		}
	}
}

bool FOnlineCognitiveServicesPlayFab::SendTextAsVoice(const FUniqueNetId& Sender, const FString& ChatText)
{
	if (Sender.IsValid() == false)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendTextAsVoice: Sender was invalid"));
		return false;
	}

	if (ChatText.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendTextAsVoice: InChatText was empty"));
		return false;
	}

	PartyLocalChatControl* LocalChatControl = GetLocalChatControl(Sender);
	if (LocalChatControl == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendTextAsVoice: Could not get LocalChatControl for User:%s"), *Sender.ToString());
		return false;
	}

	PartyError Err = LocalChatControl->SynthesizeTextToSpeech(PartySynthesizeTextToSpeechType::VoiceChat, TCHAR_TO_UTF8(*ChatText), nullptr);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendTextAsVoice failed, %s, User:%s"), *GetPartyErrorMessage(Err), *Sender.ToString());
		return false;
	}

	return true;
}

bool FOnlineCognitiveServicesPlayFab::SendChatText(const FUniqueNetId& Sender, const FString& ChatText)
{
	if (Sender.IsValid() == false)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendChatText: Sender was invalid"));
		return false;
	}

	if (ChatText.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendChatMessage: InChatText was empty"));
		return false;
	}
	
	PartyLocalChatControl* LocalChatControl = GetLocalChatControl(Sender);
	if (LocalChatControl == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendChatText: Could not get LocalChatControl for User:%s"), *Sender.ToString());
		return false;
	}

	if (OSSPlayFab == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendChatText: PlayFabPartySubsystem was null"));
		return false;
	}

	PartyNetwork* PlayFabPartyNetwork = OSSPlayFab->Network;
	if (PlayFabPartyNetwork == nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendChatText: PlayFabPartyNetwork was null"));
		return false;
	}

	uint32 Count = 0;
	PartyChatControlArray ChatControls = nullptr;
	PlayFabPartyNetwork->GetChatControls(&Count, &ChatControls);

	// Only send to remote chat controls
	TArray<PartyChatControl*> TargetControls;
	for (uint32 i = 0; i < Count; ++i)
	{
		PartyChatControl* Control = ChatControls[i];
		if (Control && Control != LocalChatControl)
		{
			PartyLocalChatControl* TempLocalControl = nullptr;
			Control->GetLocal(&TempLocalControl);
			if (TempLocalControl == nullptr)
			{
				TargetControls.Add(Control);
			}
		}
	}

	if (TargetControls.Num() > 0)
	{
		PartyError Err = LocalChatControl->SendText(TargetControls.Num(), TargetControls.GetData(), TCHAR_TO_UTF8(*ChatText), 0, nullptr);
		if (PARTY_FAILED(Err))
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::SendChatText failed, %s, User:%s"), *GetPartyErrorMessage(Err), *Sender.ToString());
			return false;
		}
	}
	
	return true;
}

void FOnlineCognitiveServicesPlayFab::OnChatTextReceived(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnChatTextReceived"));

	const PartyChatTextReceivedStateChange* Result = static_cast<const PartyChatTextReceivedStateChange*>(Change);
	if (Result)
	{
		if (Result->senderChatControl == nullptr)
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::OnChatTextReceived: senderChatControl was null"));
			return;
		}

		PartyString EntityIdStr = nullptr;
		PartyError Err = Result->senderChatControl->GetEntityId(&EntityIdStr);
		if (PARTY_FAILED(Err))
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::OnChatTextReceived: GetEntityId failed: %s\n"), *GetPartyErrorMessage(Err));
			return;
		}
		else
		{
			FString EntityId = ANSI_TO_TCHAR(EntityIdStr);
			FString ChatText = UTF8_TO_TCHAR(Result->chatText);

			for (uint32 i = 0; i < Result->translationCount; i++)
			{
				PartyTranslation& Translation = Result->translations[i];
				if (PARTY_FAILED(Translation.errorDetail))
				{
					UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::OnChatTextReceived: Translation failed %s: %s"), *GetPartyErrorMessage(Translation.errorDetail), *PartyStateChangeResultToReasonString(Translation.result));
					continue;
				}

				UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnChatTextReceived: Translation to %s: %s"), UTF8_TO_TCHAR(Translation.languageCode), UTF8_TO_TCHAR(Translation.translation));
				ChatText = Translation.translation;
			}

			FString PlatformNetIdStr = GetPlatformIdFromEntityId(EntityId);
			PLATFORM_UNIQUE_NET_ID_REF PlatformNetID = PLATFORM_UNIQUE_NET_ID::Create(PlatformNetIdStr);

			UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnChatTextReceived: PlatformNetIdStr:%s ChatText:%s"), *PlatformNetIdStr, *ChatText);

			TriggerOnPlayFabChatTextReceivedDelegates(PlatformNetID, ChatText);
		}
	}
}

void FOnlineCognitiveServicesPlayFab::OnVoiceChatTranscriptionReceived(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnVoiceChatTranscriptionReceived"));

	const PartyVoiceChatTranscriptionReceivedStateChange* Result = static_cast<const PartyVoiceChatTranscriptionReceivedStateChange*>(Change);
	if (Result)
	{
		if (Result->senderChatControl == nullptr)
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::OnVoiceChatTranscriptionReceived: senderChatControl was null"));
			return;
		}

		if (Result->result == PartyStateChangeResult::Succeeded)
		{
			PartyString EntityIdStr = nullptr;
			PartyError Err = Result->senderChatControl->GetEntityId(&EntityIdStr);
			if (PARTY_FAILED(Err))
			{
				UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::OnVoiceChatTranscriptionReceived: GetEntityId failed: %s\n"), *GetPartyErrorMessage(Err));
				return;
			}
			else
			{
				FString EntityId = ANSI_TO_TCHAR(EntityIdStr);
				EVoiceChatTranscriptionSource Source = Result->sourceType == PartyAudioSourceType::Microphone ? EVoiceChatTranscriptionSource::Microphone : EVoiceChatTranscriptionSource::TextToSpeech;
				bool bFinalTranscription = Result->type == PartyVoiceChatTranscriptionPhraseType::Final ? true : false;
				FString TranscriptionText = UTF8_TO_TCHAR(Result->transcription);

				for (uint32 i = 0; i < Result->translationCount; i++)
				{
					PartyTranslation& Translation = Result->translations[i];
					if (PARTY_FAILED(Translation.errorDetail))
					{
						UE_LOG_ONLINE(Warning, TEXT("FOnlineCognitiveServicesPlayFab::OnVoiceChatTranscriptionReceived: Translation failed %s: %s"), *GetPartyErrorMessage(Translation.errorDetail), *PartyStateChangeResultToReasonString(Translation.result));
						continue;
					}

					UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnVoiceChatTranscriptionReceived: Translation to %s: %s"), UTF8_TO_TCHAR(Translation.languageCode), UTF8_TO_TCHAR(Translation.translation));
					TranscriptionText = Translation.translation;
				}

				FString PlatformNetIdStr = GetPlatformIdFromEntityId(EntityId);
				PLATFORM_UNIQUE_NET_ID_REF PlatformNetID = PLATFORM_UNIQUE_NET_ID::Create(PlatformNetIdStr);

				UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnVoiceChatTranscriptionReceived: PlatformNetIdStr:%s TranscriptionSource:%s bFinalTranscription:%s TranscriptionText:%s"), 
					*PlatformNetIdStr,
					Source == EVoiceChatTranscriptionSource::Microphone ? TEXT("Microphone") : TEXT("TextToSpeech"),
					bFinalTranscription ? TEXT("true") : TEXT("false"), 
					*TranscriptionText);

				TriggerOnPlayFabVoiceChatTranscriptionReceivedDelegates(PlatformNetID, Source, bFinalTranscription, TranscriptionText);
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("OnVoiceChatTranscriptionReceived: FAIL:  %s"), *PartyStateChangeResultToReasonString(Result->result));
			UE_LOG_ONLINE(Warning, TEXT("ErrorDetail: %s"), *GetPartyErrorMessage(Result->errorDetail));
		}
	}
}

void FOnlineCognitiveServicesPlayFab::OnSetTextToSpeechProfileCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnSetTextToSpeechProfileCompleted"));

	const PartySetTextToSpeechProfileCompletedStateChange* Result = static_cast<const PartySetTextToSpeechProfileCompletedStateChange*>(Change);
	if (Result)
	{
		if (Result->result != PartyStateChangeResult::Succeeded)
		{
			UE_LOG_ONLINE(Warning, TEXT("OnSetTextToSpeechProfileCompleted: FAIL:  %s"), *PartyStateChangeResultToReasonString(Result->result));
			UE_LOG_ONLINE(Warning, TEXT("ErrorDetail: %s"), *GetPartyErrorMessage(Result->errorDetail));
		}
	}
}

void FOnlineCognitiveServicesPlayFab::OnSynthesizeTextToSpeechCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnSynthesizeTextToSpeechCompleted"));

	const PartySynthesizeTextToSpeechCompletedStateChange* Result = static_cast<const PartySynthesizeTextToSpeechCompletedStateChange*>(Change);
	if (Result)
	{
		if (Result->result != PartyStateChangeResult::Succeeded)
		{
			UE_LOG_ONLINE(Warning, TEXT("OnTextSynthesizeCompleted: FAIL:  %s"), *PartyStateChangeResultToReasonString(Result->result));
			UE_LOG_ONLINE(Warning, TEXT("ErrorDetail: %s"), *GetPartyErrorMessage(Result->errorDetail));
		}
	}
}

void FOnlineCognitiveServicesPlayFab::OnSetTranscriptionOptionsCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnSetTranscriptionOptionsCompleted"));

	const PartySetTranscriptionOptionsCompletedStateChange* Result = static_cast<const PartySetTranscriptionOptionsCompletedStateChange*>(Change);
	if (Result)
	{
		if (Result->result != PartyStateChangeResult::Succeeded)
		{
			UE_LOG_ONLINE(Warning, TEXT("OnSetTranscriptionOptionsCompleted: FAIL:  %s"), *PartyStateChangeResultToReasonString(Result->result));
			UE_LOG_ONLINE(Warning, TEXT("ErrorDetail: %s"), *GetPartyErrorMessage(Result->errorDetail));
		}
	}
}

void FOnlineCognitiveServicesPlayFab::OnSetTextChatOptionsCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnSetTextChatOptionsCompleted"));

	const PartySetTextChatOptionsCompletedStateChange* Result = static_cast<const PartySetTextChatOptionsCompletedStateChange*>(Change);
	if (Result)
	{
		if (Result->result != PartyStateChangeResult::Succeeded)
		{
			UE_LOG_ONLINE(Warning, TEXT("OnSetTextChatOptionsCompleted: FAIL:  %s"), *PartyStateChangeResultToReasonString(Result->result));
			UE_LOG_ONLINE(Warning, TEXT("ErrorDetail: %s"), *GetPartyErrorMessage(Result->errorDetail));
		}
	}
}

void FOnlineCognitiveServicesPlayFab::OnPopulateAvailableTextToSpeechProfilesCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineCognitiveServicesPlayFab::OnPopulateAvailableTextToSpeechProfilesCompleted"));

	const PartyPopulateAvailableTextToSpeechProfilesCompletedStateChange* Result = static_cast<const PartyPopulateAvailableTextToSpeechProfilesCompletedStateChange*>(Change);
	if (Result)
	{
		if (Result->result != PartyStateChangeResult::Succeeded)
		{
			UE_LOG_ONLINE(Warning, TEXT("OnPopulateAvailableTextToSpeechProfilesCompleted: FAIL:  %s"), *PartyStateChangeResultToReasonString(Result->result));
			UE_LOG_ONLINE(Warning, TEXT("ErrorDetail: %s"), *GetPartyErrorMessage(Result->errorDetail));
		}
	}
}

PartyLocalChatControl* FOnlineCognitiveServicesPlayFab::GetLocalChatControl(const FUniqueNetId& Sender)
{
	PartyLocalChatControl* LocalChatControl = nullptr;
	
	if (OSSPlayFab)
	{
		IOnlineVoicePtr VoiceIntPtr = OSSPlayFab->GetVoiceInterface();
		if (VoiceIntPtr.IsValid())
		{
			FOnlineVoicePlayFab* PlayFabVoiceInt = static_cast<FOnlineVoicePlayFab*>(VoiceIntPtr.Get());
			if (PlayFabVoiceInt)
			{
				LocalChatControl = PlayFabVoiceInt->GetPartyLocalChatControl(Sender);
			}
		}
	}

	return LocalChatControl;
}

FString FOnlineCognitiveServicesPlayFab::GetPlatformIdFromEntityId(const FString& EntityId)
{
	FString PlatformUserId;

	if (OSSPlayFab)
	{
		IOnlineVoicePtr VoiceIntPtr = OSSPlayFab->GetVoiceInterface();
		if (VoiceIntPtr.IsValid())
		{
			FOnlineVoicePlayFab* PlayFabVoiceInt = static_cast<FOnlineVoicePlayFab*>(VoiceIntPtr.Get());
			if (PlayFabVoiceInt)
			{
				PlatformUserId = PlayFabVoiceInt->GetPlatformIdFromEntityId(EntityId);
			}
		}
	}

	return PlatformUserId;
}