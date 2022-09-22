//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "OnlineVoiceInterfacePlayFab.h"
#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"

FOnlineVoicePlayFab::FOnlineVoicePlayFab(class FOnlineSubsystemPlayFab* InSubsystem) :
	OSSPlayFab(InSubsystem)
{

}

FOnlineVoicePlayFab::~FOnlineVoicePlayFab()
{
}

PartyLocalChatControl* FOnlineVoicePlayFab::GetPartyLocalChatControl(const FUniqueNetId& UserId) const
{
	if (UserId.IsValid())
	{
		const FString& PlatformId = UserId.ToString();
		if (LocalTalkers.Contains(PlatformId))
		{
			const FLocalTalkerPlayFab& PartyLocalTalker = LocalTalkers[PlatformId];
			return PartyLocalTalker.GetChatControl();
		}
	}

	return nullptr;
}

void FOnlineVoicePlayFab::OnAppSuspend()
{
	UE_LOG_ONLINE_VOICE(Verbose, TEXT("FOnlineVoicePlayFab::OnAppSuspend"));

	LocalTalkers.Empty();
	RemoteTalkers.Empty();
	
	CleanUpPartyXblManager();
}

void FOnlineVoicePlayFab::OnAppResume()
{
	UE_LOG_ONLINE_VOICE(Verbose, TEXT("FOnlineVoicePlayFab::OnAppResume"));

	InitPartyXblManager();
}

const FLocalTalkerPlayFab* FOnlineVoicePlayFab::GetLocalTalker(const int32 LocalUserNum) const
{
	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (IdentityIntPtr.IsValid())
	{
		TSharedPtr<const FUniqueNetId> LocalUserId = IdentityIntPtr->GetUniquePlayerId(LocalUserNum);
		if (LocalUserId.IsValid())
		{
			const FString& PlatformId = LocalUserId->ToString();
			if (LocalTalkers.Contains(PlatformId))
			{
				const FLocalTalkerPlayFab& PartyLocalTalker = LocalTalkers[PlatformId];
				return &PartyLocalTalker;
			}
		}
	}

	return nullptr;
}

const FRemoteTalkerPlayFab* FOnlineVoicePlayFab::GetRemoteTalker(const FUniqueNetId& UniqueId) const
{
	const FString& PlatformId = UniqueId.ToString();
	if (LocalTalkers.Contains(PlatformId))
	{
		const FRemoteTalkerPlayFab& PartyLocalTalker = RemoteTalkers[PlatformId];
		return &PartyLocalTalker;
	}

	return nullptr;
}

void FOnlineVoicePlayFab::StartNetworkedVoice(uint8 LocalUserNum)
{
	const FLocalTalkerPlayFab* LocalTalker = GetLocalTalker(LocalUserNum);
	if (LocalTalker)
	{
		PartyLocalChatControl* ChatControl = LocalTalker->GetChatControl();
		if (ChatControl)
		{
			ChatControl->SetAudioInputMuted(false);
		}
	}
}

void FOnlineVoicePlayFab::StopNetworkedVoice(uint8 LocalUserNum)
{
	const FLocalTalkerPlayFab* LocalTalker = GetLocalTalker(LocalUserNum);
	if (LocalTalker)
	{
		PartyLocalChatControl* ChatControl = LocalTalker->GetChatControl();
		if (ChatControl)
		{
			ChatControl->SetAudioInputMuted(true);
		}
	}
}

bool FOnlineVoicePlayFab::RegisterLocalTalker(uint32 LocalUserNum)
{
	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (IdentityIntPtr.IsValid())
	{
		FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());

		TSharedPtr<const FUniqueNetId> LocalUserId = IdentityIntPtr->GetUniquePlayerId(LocalUserNum);
		if (!LocalUserId.IsValid())
		{
			UE_LOG_ONLINE_VOICE(Warning, TEXT("RegisterLocalTalker: Unable to register local talker %u, unable to get unique player id"), LocalUserNum);
			return false;
		}
		else
		{
			TSharedPtr<FPlayFabUser> pLocalPartyUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(*LocalUserId);
			return RegisterLocalTalker(pLocalPartyUser);
		}
	}

	return false;
}

bool FOnlineVoicePlayFab::RegisterLocalTalker(const FUniqueNetId& LocalPlayer)
{
	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	if (IdentityIntPtr.IsValid())
	{
		FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());

		TSharedPtr<FPlayFabUser> pLocalPartyUser = PlayFabIdentityInt->GetPartyLocalUserFromPlatformId(LocalPlayer);
		return RegisterLocalTalker(pLocalPartyUser);
	}

	return false;
}

bool FOnlineVoicePlayFab::RegisterLocalTalker(TSharedPtr<FPlayFabUser> LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		return false;
	}

	PartyManager& Manager = PartyManager::GetSingleton();

	// Get the local chat user
	Party::PartyLocalUser* ExistingLocalUser = LocalPlayer->GetPartyLocalUser();

	PartyLocalDevice* LocalDevice = nullptr;

	// Get local device
	PartyError Err = Manager.GetLocalDevice(&LocalDevice);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("PartyManager::GetLocalDevice failed: %s"), *GetPartyErrorMessage(Err));
		return false;
	}

	Party::PartyLocalChatControl* NewChatControl = nullptr;

	// Create a chat control for the local user on this device
	Err = LocalDevice->CreateChatControl(
		ExistingLocalUser,
		LocalChatControlLanguage.IsEmpty() ? nullptr : TCHAR_TO_UTF8(*LocalChatControlLanguage),
		nullptr,
		&NewChatControl
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("PartyManager::CreateChatControl failed: %s"), *GetPartyErrorMessage(Err));
		return false;
	}

	PartyAudioDeviceSelectionType AudioDeviceSelectionType = PartyAudioDeviceSelectionType::PlatformUserDefault;
	std::string PlatformUserId = TCHAR_TO_UTF8(*LocalPlayer->GetPlatformUserId());
	PartyString AudioDeviceSelectionContext = PlatformUserId.c_str();
#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_WIN64)
	AudioDeviceSelectionType = PartyAudioDeviceSelectionType::SystemDefault;
	AudioDeviceSelectionContext = nullptr;
#elif defined(OSS_PLAYFAB_PLAYSTATION)
	if (!GetPlatformAudioDevice(AudioDeviceSelectionContext, PlatformUserId))
	{
		return false;
	}
#endif // OSS_PLAYFAB_PLAYSTATION

	// Use platform default settings for user for audio in and out devices
	Err = NewChatControl->SetAudioInput(
		AudioDeviceSelectionType,
		AudioDeviceSelectionContext,
		nullptr
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("PartyManager::SetAudioInput failed: %s"), *GetPartyErrorMessage(Err));
	}

	Err = NewChatControl->SetAudioOutput(
		AudioDeviceSelectionType,
		AudioDeviceSelectionContext,
		nullptr
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("PartyManager::SetAudioOutput failed: %s"), *GetPartyErrorMessage(Err));
	}

	// Get the available list of text to speech profiles
	Err = NewChatControl->PopulateAvailableTextToSpeechProfiles(nullptr);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("Populating available TextToSpeechProfiles failed: %s"), *GetPartyErrorMessage(Err));
	}

#if OSS_PLAYFAB_IS_PC
	// Set the device to muted by default, this is standard for push to talk behavior on PC
	NewChatControl->SetAudioInputMuted(true);
#else
	// On console, unmute by default since the device manages this via voice activity detection
	NewChatControl->SetAudioInputMuted(false);
#endif
	
	// Add the chat control into the network mesh
	if (!OSSPlayFab->AddChatControlToNetwork(NewChatControl))
	{
		return false;
	}

	// NOTE: We store using the platform user id (e.g. xuid on xbox) instead, since its easier to retrieve when the person speaks due to UE giving us the base platform user id
	FLocalTalkerPlayFab NewTalkerInst(LocalPlayer->GetPlatformUserId(), LocalDevice, NewChatControl);
	LocalTalkers.Emplace(LocalPlayer->GetPlatformUserId(), NewTalkerInst);

	StartTrackingPermissionForTalker(LocalPlayer->GetPlatformUserId(), false /*IsRemote*/);

	// Always update permissions for controls when were done here, order of local + remote may not be guaranteed
	UpdatePermissionsForAllControls();

	return true;
}

void FOnlineVoicePlayFab::RegisterLocalTalkers()
{
	if (OSSPlayFab)
	{
		IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
		if (IdentityIntPtr.IsValid())
		{
			FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
			if (PlayFabIdentityInt)
			{
				const TArray<TSharedPtr<FPlayFabUser>>& ArrUsers = PlayFabIdentityInt->GetAllPartyLocalUsers();
				for (TSharedPtr<FPlayFabUser> User : ArrUsers)
				{
					RegisterLocalTalker(User);
				}
			}
		}
	}
}

bool FOnlineVoicePlayFab::UnregisterLocalTalker(const FUniqueNetId& PlayerId)
{
	const FString& PlatformId = PlayerId.ToString();
	if (LocalTalkers.Contains(PlatformId))
	{
		const FLocalTalkerPlayFab& LocalTalker = LocalTalkers[PlatformId];
		return UnregisterLocalTalker(&LocalTalker);
	}

	return false;
}

bool FOnlineVoicePlayFab::UnregisterLocalTalker(uint32 LocalUserNum)
{
	const FLocalTalkerPlayFab* LocalTalker = GetLocalTalker(LocalUserNum);
	if (LocalTalker)
	{
		return UnregisterLocalTalker(LocalTalker);
	}

	return false;
}

bool FOnlineVoicePlayFab::UnregisterLocalTalker(const FLocalTalkerPlayFab* LocalTalker)
{
	PartyLocalDevice* LocalDevice = nullptr;

	// Get local device
	PartyError Err = PartyManager::GetSingleton().GetLocalDevice(&LocalDevice);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("PartyManager::GetLocalDevice failed: %s"), *GetPartyErrorMessage(Err));
	}

	Err = LocalDevice->DestroyChatControl(LocalTalker->GetChatControl(), nullptr);
	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("PartyManager::DestroyChatControl failed: %s"), *GetPartyErrorMessage(Err));
	}

	if (OnPlayerTalkingStateChangedDelegates.IsBound())
	{
		PLATFORM_UNIQUE_NET_ID_REF PlatformNetID = PLATFORM_UNIQUE_NET_ID::Create(LocalTalker->GetPlatformUserId());
		OnPlayerTalkingStateChangedDelegates.Broadcast(PlatformNetID, false);
	}

	LocalTalkers.Remove(LocalTalker->GetPlatformUserId());

	return true;
}

void FOnlineVoicePlayFab::UnregisterLocalTalkers()
{
	PartyLocalDevice* LocalDevice = nullptr;

	// Get local device
	PartyError Err = PartyManager::GetSingleton().GetLocalDevice(&LocalDevice);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("PartyManager::GetLocalDevice failed: %s"), *GetPartyErrorMessage(Err));
	}

	for (auto& LocalTalkerKvPair : LocalTalkers)
	{
		const FLocalTalkerPlayFab& LocalTalker = LocalTalkerKvPair.Value;
		Err = LocalDevice->DestroyChatControl(LocalTalker.GetChatControl(), nullptr);
		if (PARTY_FAILED(Err))
		{
			UE_LOG_ONLINE(Warning, TEXT("PartyManager::DestroyChatControl failed: %s"), *GetPartyErrorMessage(Err));
		}

		if (OnPlayerTalkingStateChangedDelegates.IsBound())
		{
			PLATFORM_UNIQUE_NET_ID_REF PlatformNetID = PLATFORM_UNIQUE_NET_ID::Create(LocalTalker.GetPlatformUserId());
			OnPlayerTalkingStateChangedDelegates.Broadcast(PlatformNetID, false);
		}

		StopTrackingPermissionForTalker(LocalTalkerKvPair.Key);
	}

	LocalTalkers.Empty();
}

bool FOnlineVoicePlayFab::RegisterRemoteTalker(const FUniqueNetId& UniqueId)
{
	StartTrackingPermissionForTalker(UniqueId.ToString(), true);
	return true;
}

bool FOnlineVoicePlayFab::UnregisterRemoteTalker(const FUniqueNetId& UniqueId)
{
	const FRemoteTalkerPlayFab* RemoteTalker = GetRemoteTalker(UniqueId);
	if (RemoteTalker)
	{
		if (OnPlayerTalkingStateChangedDelegates.IsBound() && (RemoteTalker->bIsTalking || RemoteTalker->bWasTalking))
		{
			PLATFORM_UNIQUE_NET_ID_REF PlatformNetID = PLATFORM_UNIQUE_NET_ID::Create(RemoteTalker->GetPlatformUserId());
			OnPlayerTalkingStateChangedDelegates.Broadcast(RemoteTalker->TalkerId.ToSharedRef(), false);
		}

		// Remotes get destroyed automatically, so we just remove it from our cache here
		RemoteTalkers.Remove(UniqueId.ToString());
		StopTrackingPermissionForTalker(UniqueId.ToString());

		return true;
	}

	return false;
}

void FOnlineVoicePlayFab::RemoveAllRemoteTalkers()
{
	for (auto& RemoteTalkerKvPair : RemoteTalkers)
	{
		const FRemoteTalkerPlayFab& RemoteTalker = RemoteTalkerKvPair.Value;
		
		if (OnPlayerTalkingStateChangedDelegates.IsBound() && (RemoteTalker.bIsTalking || RemoteTalker.bWasTalking))
		{
			PLATFORM_UNIQUE_NET_ID_REF PlatformNetID = PLATFORM_UNIQUE_NET_ID::Create(RemoteTalker.GetPlatformUserId());
			OnPlayerTalkingStateChangedDelegates.Broadcast(PlatformNetID, false);
		}
	}

	// Remotes get destroyed automatically, so we just wipe our cache here
	RemoteTalkers.Empty();
}

bool FOnlineVoicePlayFab::IsHeadsetPresent(uint32 LocalUserNum)
{
	const FLocalTalkerPlayFab* LocalTalker = GetLocalTalker(LocalUserNum);
	if (LocalTalker)
	{
		PartyAudioDeviceSelectionType DeviceType;
		PartyString DeviceContext, DeviceId;
		PartyError Err = LocalTalker->GetChatControl()->GetAudioInput(&DeviceType, &DeviceContext, &DeviceId);
		if (PARTY_SUCCEEDED(Err))
		{
			FString DeviceIdString = DeviceId;
			if (DeviceIdString.IsEmpty())
			{
				return false;
			}
			else
			{
				return true;
			}
		}
	}

	return false;
}

bool FOnlineVoicePlayFab::IsLocalPlayerTalking(uint32 LocalUserNum)
{
	const FLocalTalkerPlayFab* LocalTalker = GetLocalTalker(LocalUserNum);
	if (LocalTalker)
	{
		return LocalTalker->bIsTalking;
	}

	return false;
}

bool FOnlineVoicePlayFab::IsRemotePlayerTalking(const FUniqueNetId& UniqueId)
{
	const FRemoteTalkerPlayFab* RemoteTalker = GetRemoteTalker(UniqueId);
	if (RemoteTalker)
	{
		return RemoteTalker->bIsTalking;
	}

	return false;
}

bool FOnlineVoicePlayFab::IsMuted(uint32 LocalUserNum, const FUniqueNetId& UniqueId) const
{
	const FLocalTalkerPlayFab* LocalTalker = GetLocalTalker(LocalUserNum);
	if (LocalTalker)
	{
		PartyBool InputMuted;
		PartyError Err = LocalTalker->GetChatControl()->GetAudioInputMuted(&InputMuted);
		if (PARTY_SUCCEEDED(Err))
		{
			return static_cast<bool>(InputMuted);
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("GetAudioInputMuted failed: %s"), *GetPartyErrorMessage(Err));
		}
	}

	return false;
}

bool FOnlineVoicePlayFab::MuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide)
{
	const FLocalTalkerPlayFab* LocalTalker = GetLocalTalker(LocalUserNum);
	const FRemoteTalkerPlayFab* RemoteTalker = GetRemoteTalker(PlayerId);
	if (LocalTalker && RemoteTalker)
	{
		PartyError Err = LocalTalker->GetChatControl()->SetIncomingAudioMuted(RemoteTalker->GetChatControl(), static_cast<PartyBool>(true));
		if (PARTY_SUCCEEDED(Err))
		{
			return true;
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("SetIncomingAudioMuted failed: %s"), *GetPartyErrorMessage(Err));
		}
	}

	return false;
}

bool FOnlineVoicePlayFab::UnmuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide)
{
	const FLocalTalkerPlayFab* LocalTalker = GetLocalTalker(LocalUserNum);
	const FRemoteTalkerPlayFab* RemoteTalker = GetRemoteTalker(PlayerId);
	if (LocalTalker && RemoteTalker)
	{
		PartyError Err = LocalTalker->GetChatControl()->SetIncomingAudioMuted(RemoteTalker->GetChatControl(), static_cast<PartyBool>(false));
		if (PARTY_SUCCEEDED(Err))
		{
			return true;
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("SetIncomingAudioMuted failed: %s"), *GetPartyErrorMessage(Err));
		}
	}

	return false;
}

TSharedPtr<class FVoicePacket> FOnlineVoicePlayFab::SerializeRemotePacket(FArchive& Ar)
{
	// NOTE: NOOP for PlayFab Party
	return nullptr;
}

TSharedPtr<class FVoicePacket> FOnlineVoicePlayFab::GetLocalPacket(uint32 LocalUserNum)
{
	// NOTE: NOOP for PlayFab Party
	return nullptr;
}

int32 FOnlineVoicePlayFab::GetNumLocalTalkers()
{
	return LocalTalkers.Num();
}

void FOnlineVoicePlayFab::ClearVoicePackets()
{
	// NOTE: NOOP for PlayFab Party
}

#if OSS_PLAYFAB_VERBOSE_VOIP_LOGGING
void FOnlineVoicePlayFab::DumpState()
{
	UE_LOG_ONLINE(Warning, TEXT("START LOCAL"));
	for (auto& LocalTalkerKvPair : LocalTalkers)
	{
		const FLocalTalkerPlayFab& LocalTalker = LocalTalkerKvPair.Value;
		PartyLocalChatControlChatIndicator ChatControlIndicator;
		LocalTalker.GetChatControl()->GetLocalChatIndicator(&ChatControlIndicator);

		switch (ChatControlIndicator)
		{
		case PartyLocalChatControlChatIndicator::Silent:		  UE_LOG_ONLINE(Warning, TEXT("LOCAL STATE IS Silent")); break;
		case PartyLocalChatControlChatIndicator::Talking:		  UE_LOG_ONLINE(Warning, TEXT("LOCAL STATE IS Talking")); break;
		case PartyLocalChatControlChatIndicator::AudioInputMuted: UE_LOG_ONLINE(Warning, TEXT("LOCAL STATE IS AudioInputMuted")); break;
		case PartyLocalChatControlChatIndicator::NoAudioInput:	  UE_LOG_ONLINE(Warning, TEXT("LOCAL STATE IS NoAudioInput")); break;
		}
	}
	UE_LOG_ONLINE(Warning, TEXT("END LOCAL"));

	// And now remote endpoints
	UE_LOG_ONLINE(Warning, TEXT("START REMOTE"));
	for (auto& LocalTalkerKvPair : LocalTalkers)
	{
		for (auto& RemoteTalkerKvPair : RemoteTalkers)
		{
			const FLocalTalkerPlayFab& LocalTalker = LocalTalkerKvPair.Value;
			const FRemoteTalkerPlayFab& RemoteTalker = RemoteTalkerKvPair.Value;
			PartyChatControlChatIndicator ChatControlIndicator;
			LocalTalker.GetChatControl()->GetChatIndicator(RemoteTalker.GetChatControl(), &ChatControlIndicator);

			switch (ChatControlIndicator)
			{
			case PartyChatControlChatIndicator::Silent:						 UE_LOG_ONLINE(Warning, TEXT("REMOTE STATE IS Silent")); break;
			case PartyChatControlChatIndicator::Talking:					 UE_LOG_ONLINE(Warning, TEXT("REMOTE STATE IS Talking")); break;
			case PartyChatControlChatIndicator::IncomingVoiceDisabled:		 UE_LOG_ONLINE(Warning, TEXT("REMOTE STATE IS IncomingVoiceDisabled")); break;
			case PartyChatControlChatIndicator::IncomingCommunicationsMuted: UE_LOG_ONLINE(Warning, TEXT("REMOTE STATE IS IncomingCommunicationsMuted")); break;
			}
		}
	}
	UE_LOG_ONLINE(Warning, TEXT("END REMOTE"));
}
#endif

void FOnlineVoicePlayFab::ProcessTalkingDelegates(float DeltaTime)
{
	for (auto& LocalTalkerKvPair : LocalTalkers)
	{
		FLocalTalkerPlayFab& LocalTalker = LocalTalkerKvPair.Value;
		PartyLocalChatControlChatIndicator ChatControlIndicator;
		LocalTalker.GetChatControl()->GetLocalChatIndicator(&ChatControlIndicator);

		LocalTalker.bIsTalking = ChatControlIndicator == PartyLocalChatControlChatIndicator::Talking;

		// If the talker was not previously talking, but now is trigger the event
		bool bShouldNotify = !LocalTalker.bWasTalking && LocalTalker.bIsTalking;
		// If the talker was previously talking, but now isn't time delay the event
		if (!bShouldNotify && LocalTalker.bWasTalking)
		{
			LocalTalker.LastNotificationTime -= DeltaTime;
			if (LocalTalker.LastNotificationTime <= 0.f)
			{
				// Clear the flag so it only activates when needed
				LocalTalker.bIsTalking = false;
				LocalTalker.LastNotificationTime = VoiceNotificationDelta;
				bShouldNotify = true;
			}
		}

		if (bShouldNotify)
		{
			if (OnPlayerTalkingStateChangedDelegates.IsBound())
			{
				PLATFORM_UNIQUE_NET_ID_REF PlatformNetID = PLATFORM_UNIQUE_NET_ID::Create(LocalTalker.GetPlatformUserId());
				OnPlayerTalkingStateChangedDelegates.Broadcast(PlatformNetID, LocalTalker.bIsTalking);
			}

			LocalTalker.bWasTalking = LocalTalker.bIsTalking;
			UE_LOG_ONLINE_VOICE(VeryVerbose, TEXT("Trigger is %sTALKING"), LocalTalker.bIsTalking ? TEXT("") : TEXT("NOT"));
		}
	}

	for (auto& LocalTalkerKvPair : LocalTalkers)
	{
		for (auto& RemoteTalkerKvPair : RemoteTalkers)
		{
			FLocalTalkerPlayFab& LocalTalker = LocalTalkerKvPair.Value;
			FRemoteTalkerPlayFab& RemoteTalker = RemoteTalkerKvPair.Value;
			PartyChatControlChatIndicator ChatControlIndicator;
			LocalTalker.GetChatControl()->GetChatIndicator(RemoteTalker.GetChatControl(), &ChatControlIndicator);

			RemoteTalker.bIsTalking = ChatControlIndicator == PartyChatControlChatIndicator::Talking;

			// If the talker was not previously talking, but now is trigger the event
			bool bShouldNotify = !RemoteTalker.bWasTalking && RemoteTalker.bIsTalking;
			// If the talker was previously talking, but now isn't time delay the event
			if (!bShouldNotify && RemoteTalker.bWasTalking)
			{
				RemoteTalker.LastNotificationTime -= DeltaTime;
				if (RemoteTalker.LastNotificationTime <= 0.f)
				{
					// Clear the flag so it only activates when needed
					RemoteTalker.bIsTalking = false;
					RemoteTalker.LastNotificationTime = VoiceNotificationDelta;
					bShouldNotify = true;
				}
			}

			if (bShouldNotify)
			{
				if (OnPlayerTalkingStateChangedDelegates.IsBound())
				{
					PLATFORM_UNIQUE_NET_ID_REF PlatformNetID = PLATFORM_UNIQUE_NET_ID::Create(RemoteTalker.GetPlatformUserId());
					OnPlayerTalkingStateChangedDelegates.Broadcast(PlatformNetID, RemoteTalker.bIsTalking);
				}

				RemoteTalker.bWasTalking = RemoteTalker.bIsTalking;
				UE_LOG_ONLINE_VOICE(VeryVerbose, TEXT("Trigger is %sTALKING"), RemoteTalker.bIsTalking ? TEXT("") : TEXT("NOT"));
			}
		}
	}
}

void FOnlineVoicePlayFab::Tick(float DeltaTime)
{
	TickTalkerPermissionTracking();
	ProcessTalkingDelegates(DeltaTime);
}

FString FOnlineVoicePlayFab::GetVoiceDebugState() const
{
	// NOTE: NOOP for PlayFab Party
	return TEXT("");
}

void FOnlineVoicePlayFab::OnLeavePlayFabPartyNetwork()
{
	UnregisterLocalTalkers();
	RemoveAllRemoteTalkers();
}

IVoiceEnginePtr FOnlineVoicePlayFab::CreateVoiceEngine()
{
	// NOTE: NOOP for PlayFab Party
	return nullptr;
}

void FOnlineVoicePlayFab::ProcessMuteChangeNotification()
{
	// NOTE: NOOP for PlayFab Party
}

bool FOnlineVoicePlayFab::Init()
{
	bool bHasVoiceEnabled = true;
	if (GConfig->GetBool(TEXT("OnlineSubsystemPlayFab"), TEXT("bHasPlayFabVoiceEnabled"), bHasVoiceEnabled, GEngineIni) && !bHasVoiceEnabled)
	{
		UE_LOG_ONLINE_VOICE(Log, TEXT("Voice Chat is Disabled"));
		return false;
	}

	if (!GConfig->GetFloat(TEXT("OnlineSubsystem"), TEXT("VoiceNotificationDelta"), VoiceNotificationDelta, GEngineIni))
	{
		VoiceNotificationDelta = 0.2f;
		UE_LOG_ONLINE_VOICE(Warning, TEXT("Missing VoiceNotificationDelta key in OnlineSubsystem of DefaultEngine.ini"));
	}

	return true;
}

void FOnlineVoicePlayFab::OnChatControlJoinedNetwork(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnChatControlJoinedNetwork"));
}

void FOnlineVoicePlayFab::OnChatControlLeftNetwork(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnChatControlLeftNetwork"));
}

void FOnlineVoicePlayFab::OnChatControlDestroyed(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnChatControlDestroyed"));
}

void FOnlineVoicePlayFab::OnChatControlCreated(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnChatControlCreated"));

	const PartyChatControlCreatedStateChange* Result = static_cast<const PartyChatControlCreatedStateChange*>(Change);
	if (Result)
	{
		PartyString SenderEntityId = nullptr;
		PartyError Err = Result->chatControl->GetEntityId(&SenderEntityId);

		if (PARTY_FAILED(Err))
		{
			UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnChatControlCreated chatControl::GetEntityId failed: %s"), *GetPartyErrorMessage(Err));
		}
		else
		{
			// Is it remote? We already stored the local ones, so we only care about storing remote ones
			PartyLocalChatControl* LocalChatControl = nullptr;
			Err = Result->chatControl->GetLocal(&LocalChatControl);

			if (PARTY_FAILED(Err))
			{
				UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnChatControlCreated chatControl::GetLocal failed: %s"), *GetPartyErrorMessage(Err));
			}
			else
			{
				if (LocalChatControl == nullptr) // Its remote
				{
					// Store it
					FRemoteTalkerPlayFab NewTalkerInst(Result->chatControl);
					RemoteTalkers.Emplace(SenderEntityId, NewTalkerInst);
				}
			}
		}

		// Always update permissions for controls when were done here, order of local + remote may not be guaranteed
		UpdatePermissionsForAllControls();
	}
}

void FOnlineVoicePlayFab::OnChatPermissionsChanged(const FString& LocalUserId, const FString& RemoteUserId, PartyChatPermissionOptions perms)
{
	UE_LOG_ONLINE(Verbose, TEXT("OnChatPermissionChanged %s %s %d"), *LocalUserId, *RemoteUserId, perms);

	// Rebuild the relationship map
	UpdatePermissionsForAllControls();
}

void FOnlineVoicePlayFab::UpdatePermissionsForAllControls()
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::UpdatePermissionsForAllControls"));

	// first check if we have resolved a platform ID for any PF entity which did not have one associated
	for (auto& RemoteTalkerKvPair : RemoteTalkers)
	{
		FRemoteTalkerPlayFab& RemoteTalker = RemoteTalkerKvPair.Value;
		if (RemoteTalker.GetPlatformUserId().IsEmpty())
		{
			// do we have a platform ID for this user now?
			FString PlatformUserId = GetPlatformIdFromEntityId(RemoteTalkerKvPair.Key);

			if (!PlatformUserId.IsEmpty())
			{
				RemoteTalker.UpdatePlatformUserId(PlatformUserId);
			}
		}
	}

	// set permissions on all of our local chat controls
	bool bErr = false;
	for (auto& LocalTalkerKvPair : LocalTalkers)
	{
		for (auto& RemoteTalkerKvPair : RemoteTalkers)
		{
			const FLocalTalkerPlayFab& LocalTalker = LocalTalkerKvPair.Value;
			const FRemoteTalkerPlayFab& RemoteTalker = RemoteTalkerKvPair.Value;

			const FString& RemotePlatformID = RemoteTalker.GetPlatformUserId();

			if (RemotePlatformID.IsEmpty())
			{
				UE_LOG_ONLINE(Verbose, TEXT("Skipping ChatPermissions for PlayFab Entity %s at this time - the platform ID is still pending resolve"), *RemoteTalkerKvPair.Key);
				continue;
			}

			PartyChatPermissionOptions Options = GetChatPermissionsForTalker(LocalTalkerKvPair.Key, RemotePlatformID);

			UE_LOG_ONLINE(Verbose, TEXT("ChatPermissions for %s vs. %s (PlayFab EntityID: %s): %d"), *LocalTalkerKvPair.Key, *RemotePlatformID, *RemoteTalkerKvPair.Key, Options);

			PartyError Err = LocalTalker.GetChatControl()->SetPermissions(RemoteTalker.GetChatControl(), Options);

			if (PARTY_FAILED(Err))
			{
				UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnChatControlCreated chatControl::SetPermissions failed: %s"), *GetPartyErrorMessage(Err));
			}
		}
	}
}

void FOnlineVoicePlayFab::OnConnectChatControlCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnConnectChatControlCompleted"));
}

void FOnlineVoicePlayFab::OnCreateChatControlCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnCreateChatControlCompleted"));
}

void FOnlineVoicePlayFab::OnDestroyChatControlCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnDestroyChatControlCompleted"));
}

void FOnlineVoicePlayFab::OnLocalChatAudioInputChanged(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnLocalChatAudioInputChanged"));

	const auto Result = static_cast<const PartyLocalChatAudioInputChangedStateChange*>(Change);

	UE_LOG_ONLINE(Verbose, TEXT("PartyAudioInputState: %d, %s"), Result->state, *GetPartyErrorMessage(Result->errorDetail));
}

void FOnlineVoicePlayFab::OnSetChatAudioInputCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnSetChatAudioInputCompleted"));

	const auto Result = static_cast<const PartySetChatAudioInputCompletedStateChange*>(Change);

	UE_LOG_ONLINE(Verbose, TEXT("PartyStateChangeResult: %d, %hs, %s"), Result->result, Result->audioDeviceSelectionContext, *GetPartyErrorMessage(Result->errorDetail));
}

void FOnlineVoicePlayFab::OnSetChatAudioOutputCompleted(const PartyStateChange* Change)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineVoicePlayFab::OnSetChatAudioOutputCompleted"));
	
	const auto Result = static_cast<const PartySetChatAudioOutputCompletedStateChange*>(Change);

	UE_LOG_ONLINE(Verbose, TEXT("PartyStateChangeResult: %d, %hs, %s"), Result->result, Result->audioDeviceSelectionContext, *GetPartyErrorMessage(Result->errorDetail));
}