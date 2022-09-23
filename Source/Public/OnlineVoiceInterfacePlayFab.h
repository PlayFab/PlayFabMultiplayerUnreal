//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/VoiceInterface.h"
#include "OnlineSubsystemPlayFabDefines.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "OnlineIdentityInterfacePlayFab.h"
#include "Misc/CoreDelegates.h"

THIRD_PARTY_INCLUDES_START
#if defined(OSS_PLAYFAB_SWITCH) || defined(OSS_PLAYFAB_PLAYSTATION)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#include <PartyPal.h>
#include <PFMultiplayerPal.h>
#endif // OSS_PLAYFAB_SWITCH || OSS_PLAYFAB_PLAYSTATION
#include <Party.h>
using namespace Party;
THIRD_PARTY_INCLUDES_END

class FOnlineSubsystemPlayFab;

enum class ECrossNetworkType
{
	// Disable voice chat.
	// To set disabled voice chat platforms, platfroms must be listed in [OnlineSubsystemPlayFabVoiceChatDisabledPlatforms] section in ini configuration file.
	DISABLED,
	// GDK platforms which use XBL.
	// This platform types are allowed all permission on NON-GDK platforms by default.
	// And, On GDK platforms, they follows XBL permission policy.
	GDK,
	// Non-GDK platforms which doesn't use XBL.
	// This platform types are allowed all permission on NON-GDK platforms by default.
	// And, On GDK platforms, they follows XBL CrossNetwork permission policy.
	NONGDK
};

struct FCrossNetworkTalkerPlayFab
{
	ECrossNetworkType Type = ECrossNetworkType::DISABLED;
	FString RemoteUserId;
	FString PlatformModel;
};

class FLocalTalkerPlayFab : public FLocalTalker
{
public:
	FLocalTalkerPlayFab(const FString& InPlatformUserId, PartyLocalDevice* InLocalDevice, PartyLocalChatControl* InLocalChatControl)
		: PlatformUserId(InPlatformUserId),
		LocalDevice(InLocalDevice),
		LocalChatControl(InLocalChatControl)
	{
		// All talkers, if they exist, are registered
		bIsRegistered = true;
	}

	FString GetPlatformUserId() const 
	{
		return PlatformUserId;
	}

	PartyLocalDevice* GetDevice() const
	{
		return LocalDevice;
	}

	PartyLocalChatControl* GetChatControl() const
	{
		return LocalChatControl;
	}

private:
	FString PlatformUserId;
	PartyLocalDevice* LocalDevice = nullptr;
	PartyLocalChatControl* LocalChatControl = nullptr;
};

class FRemoteTalkerPlayFab : public FRemoteTalker
{
public:
	FRemoteTalkerPlayFab(const FString& InPlatformUserId, PartyChatControl* InChatControl)
		: PlatformUserId(InPlatformUserId),
		ChatControl(InChatControl)
	{
		
	}

	FRemoteTalkerPlayFab(PartyChatControl* InChatControl)
		: PlatformUserId(FString()),
		ChatControl(InChatControl)
	{

	}

	void UpdatePlatformUserId(const FString& InPlatformUserId)
	{
		PlatformUserId = InPlatformUserId;
	}

	FString GetPlatformUserId() const
	{
		return PlatformUserId;
	}

	PartyChatControl* GetChatControl() const
	{
		return ChatControl;
	}

private:
	FString PlatformUserId;
	PartyChatControl* ChatControl = nullptr;
};

class FOnlineVoicePlayFab
	: public IOnlineVoice
	, public TSharedFromThis<FOnlineVoicePlayFab, ESPMode::ThreadSafe>
{
	class FOnlineSubsystemPlayFab* OSSPlayFab;

PACKAGE_SCOPE:
	// Re-evaluates the muting list for all local talkers
	void ProcessMuteChangeNotification() override;

	//Initialize the voice interface
	virtual bool Init() override;

public:
	// Constructors
	FOnlineVoicePlayFab() = delete;
	FOnlineVoicePlayFab(class FOnlineSubsystemPlayFab* InSubsystem);

	// Virtual destructor to force proper child cleanup
	virtual ~FOnlineVoicePlayFab();

	virtual void StartNetworkedVoice(uint8 LocalUserNum) override;
	virtual void StopNetworkedVoice(uint8 LocalUserNum) override;
	virtual bool RegisterLocalTalker(uint32 LocalUserNum) override;
	bool RegisterLocalTalker(TSharedPtr<FPlayFabUser> LocalPlayer);
	bool RegisterLocalTalker(const FUniqueNetId& LocalPlayer);
	virtual void RegisterLocalTalkers() override;
	virtual bool UnregisterLocalTalker(uint32 LocalUserNum) override;
	bool UnregisterLocalTalker(const FUniqueNetId& PlayerId);
	bool UnregisterLocalTalker(const FLocalTalkerPlayFab* LocalTalker);
	virtual void UnregisterLocalTalkers() override;
	virtual bool RegisterRemoteTalker(const FUniqueNetId& UniqueId) override;
	virtual bool UnregisterRemoteTalker(const FUniqueNetId& UniqueId) override;
	virtual void RemoveAllRemoteTalkers() override;
	virtual bool IsHeadsetPresent(uint32 LocalUserNum) override;
	virtual bool IsLocalPlayerTalking(uint32 LocalUserNum) override;
	virtual bool IsRemotePlayerTalking(const FUniqueNetId& UniqueId) override;
	bool IsMuted(uint32 LocalUserNum, const FUniqueNetId& UniqueId) const override;
	bool MuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide) override;
	bool UnmuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide) override;
	virtual TSharedPtr<class FVoicePacket> SerializeRemotePacket(FArchive& Ar) override;
	virtual TSharedPtr<class FVoicePacket> GetLocalPacket(uint32 LocalUserNum) override;
	virtual int32 GetNumLocalTalkers() override;
	virtual void ClearVoicePackets() override;
	virtual void Tick(float DeltaTime) override;
	virtual FString GetVoiceDebugState() const override;

	void OnLeavePlayFabPartyNetwork();

	// PlayFab Party Network Events
	void OnChatControlJoinedNetwork(const PartyStateChange* Change);
	void OnChatControlLeftNetwork(const PartyStateChange* Change);
	void OnChatControlDestroyed(const PartyStateChange* Change);
	void OnChatControlCreated(const PartyStateChange* Change);
	void OnConnectChatControlCompleted(const PartyStateChange* Change);
	void OnCreateChatControlCompleted(const PartyStateChange* Change);
	void OnDestroyChatControlCompleted(const PartyStateChange* Change);
	void OnLocalChatAudioInputChanged(const PartyStateChange* Change);
	void OnSetChatAudioInputCompleted(const PartyStateChange* Change);
	void OnSetChatAudioOutputCompleted(const PartyStateChange* Change);

	PartyLocalChatControl* GetPartyLocalChatControl(const FUniqueNetId& UserId) const;

	FString GetPlatformIdFromEntityId(const FString& EntityId);

	void OnAppSuspend();
	void OnAppResume();
	
	void CleanUpPartyXblManager();
	void InitPartyXblManager();

	void AddTalkerIdMapping(const FString& EntityId, const FString& UserId);
	void SetTalkerCrossNetworkPermission(ECrossNetworkType VoiceChatType, const FString& RemoteUserId, const FString& PlatformModel);
	void UpdatePermissionsForAllControls();
#if defined(OSS_PLAYFAB_PLAYSTATION)
	void OnQueryBlockedPlayersComplete(const FUniqueNetId &UserId, bool bWasSuccessful, const FString &Error);
	bool GetPlatformAudioDevice(PartyString& AudioDeviceSelectionContext, std::string& PlatformUserId);
#endif // OSS_PLAYFAB_PLAYSTATION

protected:
	virtual IVoiceEnginePtr CreateVoiceEngine() override;

	const FLocalTalkerPlayFab* GetLocalTalker(const int32 LocalUserNum) const;
	const FRemoteTalkerPlayFab* GetRemoteTalker(const FUniqueNetId& UniqueId) const;

	// Platform-specific functions for chat permissions
	PartyChatPermissionOptions GetChatPermissionsForTalker(const FString& LocalId, const FString& RemoteId);
	void StartTrackingPermissionForTalker(const FString& UserId, bool IsRemote);
	void StopTrackingPermissionForTalker(const FString& UserId);
	void TickTalkerPermissionTracking();
	void OnChatPermissionsChanged(const FString& LocalUserId, const FString& RemoteUserId, PartyChatPermissionOptions perms);

private:
#if OSS_PLAYFAB_VERBOSE_VOIP_LOGGING
	void DumpState();
#endif

	void ProcessTalkingDelegates(float DeltaTime);

	float VoiceNotificationDelta = 0.2f;

	FString LocalChatControlLanguage;
	TMap<FString, FLocalTalkerPlayFab> LocalTalkers;
	TMap<FString, FRemoteTalkerPlayFab> RemoteTalkers; // NOTE: Key here is a PlayFab Entity ID, if you want the platform ID, access the FRemoteTalkerPlayFab member var
};

typedef TSharedPtr<FOnlineVoicePlayFab, ESPMode::ThreadSafe> FOnlineVoicePlayFabPtr;