//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "OnlineSubsystemPlayFab.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineSubsystemPlayFabPackage.h"

/// <summary>
/// The maximum allowed value for invited players.
/// </summary>
constexpr uint32_t c_PFInviteMaxPlayerCount = 32;

/** 
 * Implementation for the Live external UIs
 */
class FOnlineExternalUIPlayFab
	: public IOnlineExternalUI
	, public TSharedFromThis<FOnlineExternalUIPlayFab, ESPMode::ThreadSafe>
{
private:
	
	FCriticalSection CallbackLock;
	uint64_t InviteUISelectFromPlayers[c_PFInviteMaxPlayerCount] = {};
	uint64_t InviteUIPreSelectedPlayers[c_PFInviteMaxPlayerCount] = {};
	int32 LocalUserNumber;
	FName SessionName;
	FString ConnectionString;

PACKAGE_SCOPE:

	/** Constructor
	 *
	 * @param InSubsystem The owner of this external UI interface.
	 */
	explicit FOnlineExternalUIPlayFab(FOnlineSubsystemPlayFab* InSubsystem);

	/** Reference to the owning subsystem */
	class FOnlineSubsystemPlayFab* OSSPlayFab;
public:

	/**
	 * Destructor.
	 */
	virtual ~FOnlineExternalUIPlayFab()
	{
	}

	// IOnlineExternalUI
	virtual bool ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate = FOnLoginUIClosedDelegate()) override;
	virtual bool ShowAccountCreationUI(const int ControllerIndex, const FOnAccountCreationUIClosedDelegate& Delegate = FOnAccountCreationUIClosedDelegate()) override { /** NYI */ return false; }
	virtual bool ShowFriendsUI(int32 LocalUserNum) override;
	virtual bool ShowInviteUI(int32 LocalUserNum, FName SessionName = NAME_GameSession) override;
	virtual bool ShowAchievementsUI(int32 LocalUserNum) override;
	virtual bool ShowLeaderboardUI(const FString& LeaderboardName) override;
	virtual bool ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate = FOnShowWebUrlClosedDelegate()) override;
	virtual bool CloseWebURL() override;
	virtual bool ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate = FOnProfileUIClosedDelegate()) override;
	virtual bool ShowAccountUpgradeUI(const FUniqueNetId& UniqueId) override;
	virtual bool ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate = FOnShowStoreUIClosedDelegate()) override;
	virtual bool ShowSendMessageUI(int32 LocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate = FOnShowSendMessageUIClosedDelegate()) override;
	
#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	void HandleShowLoginUIComplete(bool bSuccess, FGDKUserHandle GDKUser, FOnLoginUIClosedDelegate Delegate);
#endif
#if !defined(OSS_PLAYFAB_PLAYSTATION)
	void HandleReadFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
#endif // !OSS_PLAYFAB_PLAYSTATION
	void HandleShowSendGameInvitesUIComplete(bool bIsSuccess);
#if !defined(OSS_PLAYFAB_PLAYSTATION)
	void HandleShowAchievementsUIComplete(bool bIsSuccess);
	void HandleShowStoreUIComplete(bool wasPurchaseMade);
	void HandleShowProfileUIComplete(bool bSuccess, const FOnProfileUIClosedDelegate Delegate);
#endif // !OSS_PLAYFAB_PLAYSTATION

	void Tick(float DeltaTime);

private:
#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	void ProcessShowPlayerPickerResults(TUniquePtr<XAsyncBlock> AsyncBlock);
	bool SendGDKPlatformInvite(const TArray<uint64_t> & PlayersToInvite) const;
#endif
};

typedef TSharedPtr<FOnlineExternalUIPlayFab, ESPMode::ThreadSafe> FOnlineExternalUIPlayFabPtr;
