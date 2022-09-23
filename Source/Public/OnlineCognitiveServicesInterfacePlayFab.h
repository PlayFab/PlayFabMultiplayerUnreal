//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "OnlineSubsystemPlayFabPackage.h"
#include "OnlineSubsystemPlayFabDefines.h"
#include "OnlineIdentityInterfacePlayFab.h"

THIRD_PARTY_INCLUDES_START
#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_PLAYSTATION)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <PartyPal.h>
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH || OSS_PLAYFAB_PLAYSTATION
#include <Party.h>
using namespace Party;
THIRD_PARTY_INCLUDES_END

enum class EVoiceChatTranscriptionSource
{
	Microphone,
	TextToSpeech
};

enum class ETextToSpeechType
{
	Narration,
	VoiceChat
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayFabChatTextReceived, TSharedRef<const FUniqueNetId> /*Sender*/, const FString& /*ChatText*/);
typedef FOnPlayFabChatTextReceived::FDelegate FOnPlayFabChatTextReceivedDelegate;

DECLARE_MULTICAST_DELEGATE_FourParams(FOnPlayFabVoiceChatTranscriptionReceived, TSharedRef<const FUniqueNetId> /*Sender*/, EVoiceChatTranscriptionSource /*SourceType*/, bool /*bFinalTranscription*/, const FString& /*TranscriptionText*/);
typedef FOnPlayFabVoiceChatTranscriptionReceived::FDelegate FOnPlayFabVoiceChatTranscriptionReceivedDelegate;

class FOnlineCognitiveServicesPlayFab
	: public TSharedFromThis<FOnlineCognitiveServicesPlayFab, ESPMode::ThreadSafe>
{
PACKAGE_SCOPE:

	explicit FOnlineCognitiveServicesPlayFab(class FOnlineSubsystemPlayFab* InSubsystem);

	class FOnlineSubsystemPlayFab* OSSPlayFab = nullptr;

public:

	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnPlayFabChatTextReceived, TSharedRef<const FUniqueNetId> /*Sender*/, const FString& /*ChatText*/);
	DEFINE_ONLINE_DELEGATE_FOUR_PARAM(OnPlayFabVoiceChatTranscriptionReceived, TSharedRef<const FUniqueNetId> /*Sender*/, EVoiceChatTranscriptionSource /*SourceType*/, bool /*bFinalTranscription*/, const FString& /*TranscriptionText*/);

	virtual ~FOnlineCognitiveServicesPlayFab() = default;

	void SetTextChatTranslationOptions(const FUniqueNetId& UserId, bool bTranslateToLocalLanguage);
	void SetVoiceChatTranscriptionOptions(const FUniqueNetId& UserId, bool bTranscribeSelf, bool bTranscribeOtherChatControlsWithMatchingLanguages, bool bTranscribeOtherChatControlsWithNonMatchingLanguages, bool bTranslateToLocalLanguage);
	void SetTextToSpeechOptions(const FUniqueNetId& UserId, ETextToSpeechType TextToSpeechType);

	bool SendTextAsVoice(const FUniqueNetId& Sender, const FString& ChatText);
	bool SendChatText(const FUniqueNetId& InSender, const FString& InChatText);

	void OnChatTextReceived(const PartyStateChange* Change);
	void OnVoiceChatTranscriptionReceived(const PartyStateChange* Change);
	void OnSetTextToSpeechProfileCompleted(const PartyStateChange* Change);
	void OnSynthesizeTextToSpeechCompleted(const PartyStateChange* Change);
	void OnSetTranscriptionOptionsCompleted(const PartyStateChange* Change);
	void OnSetTextChatOptionsCompleted(const PartyStateChange* Change);
	void OnPopulateAvailableTextToSpeechProfilesCompleted(const PartyStateChange* Change);

private:
	PartyLocalChatControl* GetLocalChatControl(const FUniqueNetId& Sender);
	FString GetPlatformIdFromEntityId(const FString& EntityId);
};

typedef TSharedPtr<class FOnlineCognitiveServicesPlayFab, ESPMode::ThreadSafe> FOnlineCognitiveServicesPlayFabPtr;