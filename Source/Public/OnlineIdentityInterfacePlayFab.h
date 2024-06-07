//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemPlayFabPackage.h"
#include "OnlineSubsystemPlayFabDefines.h"
#include "OnlineSubsystemPlayFabTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/DateTime.h"
#include "Dom/JsonObject.h"

#include <vector>
#include <string>

using namespace Party;

/**
 * Delegate used on the completion of FOnlineIdentityPlayFab::AuthenticateUser().
 *
 * @param LocalUserNum the controller number of the associated user that made the request
 * @param bWasSuccessful true if authentication was successful, false if there was an error
 * @param PlatformUserIdStr user identifier returned from the service.
 * @param ErrorStr string representing the error condition
 */
DECLARE_MULTICAST_DELEGATE_FourParams(FOnAuthenticateUserComplete, int32 /*LocalUserNum*/, bool /*bWasSuccessful*/, const FString& /*PlatformUserIdStr*/, const FString& /*ErrorStr*/);
typedef FOnAuthenticateUserComplete::FDelegate FOnAuthenticateUserCompleteDelegate;

struct UserAuthRequestData
{
	FHttpRequestPtr m_HTTPRequest;
};

class ONLINESUBSYSTEMPLAYFAB_API FPlayFabUser
{
public:
	FPlayFabUser
	(
		const FString& PlatformUserIdStrIn,
		const FString& EntityTokenIn,
		const FString& EntityIdIn,
		const FString& EntityTypeIn,
		const FString& SessionTicketIn,
		PartyLocalUser* LocalUserIn,
		const FString& PlayFabIdIn
	):
		PlatformUserIdStr(PlatformUserIdStrIn),
		EntityToken(EntityTokenIn),
		EntityId(EntityIdIn),
		EntityType(EntityTypeIn),
		SessionTicket(SessionTicketIn),
		LocalUser(LocalUserIn),
		EntityIdStr(TCHAR_TO_UTF8(*EntityIdIn)),
		EntityTypeStr(TCHAR_TO_UTF8(*EntityTypeIn)),
		PlayFabId(PlayFabIdIn)
	{
		SetNewEntityTokenUpdateTime();
	}

	const FString& GetPlatformUserId() const { return PlatformUserIdStr; }
	const FString& GetEntityToken() const { return EntityToken; }
	const FString& GetEntityId() const { return EntityId; }
	const FString& GetEntityType() const { return EntityType; }
	const FString& GetSessionTicket() const { return SessionTicket; }
	const FString& GetPlayFabId() const { return PlayFabId; }
	const FDateTime& GetEntityTokenUpdateTime() const { return EntityTokenUpdateTime; }
	PartyLocalUser* GetPartyLocalUser() const { return LocalUser; }
	PFEntityKey GetEntityKey() const { return PFEntityKey{ EntityIdStr.c_str(), EntityTypeStr.c_str() }; }

	void UpdateEntityToken(const FString& NewEntityToken);

	void SetNewEntityTokenUpdateTime();

private:
	FString PlatformUserIdStr;
	FString EntityToken;
	FString EntityId;
	FString EntityType;
	FString SessionTicket;
	PartyLocalUser* LocalUser = nullptr;
	std::string EntityIdStr;
	std::string EntityTypeStr;
	FString PlayFabId;
	FDateTime EntityTokenUpdateTime;
};

class ONLINESUBSYSTEMPLAYFAB_API FOnlineIdentityPlayFab:
	public IOnlineIdentity,
	public TSharedFromThis<FOnlineIdentityPlayFab, ESPMode::ThreadSafe>
{
PACKAGE_SCOPE:

	/** Constructor
	 *
	 * @param InSubsystem The owner of this identity interface.
	 */
	explicit FOnlineIdentityPlayFab(class FOnlineSubsystemPlayFab* InSubsystem);

	/** Reference to the owning subsystem */
	class FOnlineSubsystemPlayFab* OSSPlayFab = nullptr;

public:

	virtual ~FOnlineIdentityPlayFab();

	/**
	 * Delegate used on the completion of FOnlineIdentityPlayFab::AuthenticateUser().
	 *
	 * @param LocalUserNum the controller number of the associated user that made the request
	 * @param bWasSuccessful true if authentication was successful, false if there was an error
	 * @param PlatformUserIdStr user identifier returned from the service.
	 * @param ErrorStr string representing the error condition
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_THREE_PARAM(MAX_LOCAL_PLAYERS, OnAuthenticateUserComplete, bool /*bWasSuccessful*/, const FString& /*PlatformUserIdStr*/, const FString& /*ErrorStr*/);

	// IOnlineIdentity
	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) override;
	virtual bool Logout(int32 LocalUserNum) override;
	virtual bool AutoLogin(int32 LocalUserNum) override;
	virtual TSharedPtr<FUserOnlineAccount> GetUserAccount(const FUniqueNetId& UserId) const override;
	virtual TArray<TSharedPtr<FUserOnlineAccount> > GetAllUserAccounts() const override;
	virtual TSharedPtr<const FUniqueNetId> GetUniquePlayerId(int32 LocalUserNum) const override;
	virtual TSharedPtr<const FUniqueNetId> GetSponsorUniquePlayerId(int32 LocalUserNum) const override;
	virtual TSharedPtr<const FUniqueNetId> CreateUniquePlayerId(uint8* Bytes, int32 Size) override;
	virtual TSharedPtr<const FUniqueNetId> CreateUniquePlayerId(const FString& Str) override;
	virtual ELoginStatus::Type GetLoginStatus(int32 LocalUserNum) const override;
	virtual ELoginStatus::Type GetLoginStatus(const FUniqueNetId& UserId) const override;
	virtual FString GetPlayerNickname(int32 LocalUserNum) const override;
	virtual FString GetPlayerNickname(const FUniqueNetId& UserId) const override;
	virtual FString GetAuthToken(int32 LocalUserNum) const override;
	virtual void RevokeAuthToken(const FUniqueNetId& UserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate) override;
	virtual void GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate, EShowPrivilegeResolveUI ShowResolveUI) override;
	virtual FPlatformUserId GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const override;
	virtual FString GetAuthType() const override;

	UserAuthRequestData* GetUserAuthRequestData(const FString& PlatformUserIdStr)
	{
		if (UserAuthRequestsInFlight.Contains(PlatformUserIdStr))
		{
			return &UserAuthRequestsInFlight[PlatformUserIdStr];
		}

		return nullptr;
	}

	const FString GetUserPlatformIdStrFromRequest(FHttpRequestPtr HttpRequest)
	{
		for (auto& Elem : UserAuthRequestsInFlight)
		{
			const UserAuthRequestData& ReqData = Elem.Value;
			if (ReqData.m_HTTPRequest.Get() == HttpRequest.Get())
			{
				return Elem.Key;
			}
		}

		return FString();
	}

	UserAuthRequestData* GetUserAuthRequestDataFromRequest(FHttpRequestPtr HttpRequest)
	{
		for (auto& Elem : UserAuthRequestsInFlight)
		{
			const UserAuthRequestData& ReqData = Elem.Value;
			if (ReqData.m_HTTPRequest.Get() == HttpRequest.Get())
			{
				return &Elem.Value;
			}
		}

		return nullptr;
	}

	void Tick(float DeltaTime);
	void OnAppSuspend();
	void OnAppResume();
	void TryAuthenticateUsers();
	void CleanUpLocalUsers();

	PartyLocalUser* GetFirstPartyLocalUser();

	TSharedPtr<FPlayFabUser> GetPartyLocalUserFromPlatformId(const FUniqueNetId& PlatformNetId);
	TSharedPtr<FPlayFabUser> GetPartyLocalUserFromPlatformIdString(const FString& PlatformNetIdStr, int32* Index = nullptr);
	TSharedPtr<FPlayFabUser> GetPartyLocalUserFromEntityIdString(const FString& EntityIdString);

	const TArray<TSharedPtr<FPlayFabUser>>& GetAllPartyLocalUsers() const
	{
		return LocalPlayFabUsers;
	}

	bool IsUserLocal(const PFEntityKey& UserEntityKey);
	const TArray<PFEntityKey> GetLocalUserEntityKeys() const;
	const FString& GetLocalUserXToken() const { return LocalUserXToken; }

protected:
	bool AuthenticateUser(const FString& UserPlatformIdStr);
	void Auth_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	void CreateLocalUser(const FString& UserPlatformIdStr, const FString& EntityId, const FString& EntityType, const FString& SessionTicket, const FString& EntityToken, const FString& TokenExpiration, const FString& PlayFabId);
	void RemoveLocalUser(const FString& PlatformUserIdStr);

private:
	TArray<TSharedPtr<FPlayFabUser>> LocalPlayFabUsers;
	TSet<FString> UsersToAuth;
	TMap<FString, UserAuthRequestData> UserAuthRequestsInFlight;
	bool bRegisterAuthDelegates = true;
	bool bAuthAllUsers = true;
	float TimeSinceLastAuth = 0.0f;
	FString LocalUserXToken;

	TArray<FDelegateHandle> LoginStatusChangedDelegateHandles;
	TArray<FDelegateHandle> LoginCompleteDelegateHandles;
	TArray<FDelegateHandle> LogoutCompleteDelegateHandles;

	void RegisterAuthDelegates();
	void CleanUpAuthDelegates();

	void OnLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId);
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful);

public:
	// Platform specific, must implement for your platform!
	bool ApplyPlatformHTTPRequestData(const FString& PlatformUserIdStr, const FString& URL, const FString& RequestVerb);
	static void OnPopulatePlatformRequestDataCompleted(bool bWasSuccessful, const FString& PlatformUserIdStr, TMap<FString, FString> PlatformHeaders = TMap<FString, FString>(), TSharedPtr<FJsonObject> RequestBodyJson = TSharedPtr<FJsonObject>(nullptr));

	void FinishRequest(bool bPlatformDataSuccess, const FString& UserPlatformId, TMap<FString, FString> PlatformHeaders, TSharedPtr<FJsonObject> RequestBodyJson);

	//OnLoginChanged
	virtual FDelegateHandle AddOnLoginChangedDelegate_Handle(const FOnLoginChangedDelegate& Delegate) override;
	virtual void ClearOnLoginChangedDelegate_Handle(FDelegateHandle& Handle) override;
	virtual void TriggerOnLoginChangedDelegates(int32 LocalUserNum) override;

	//OnLoginStatusChanged
	virtual FDelegateHandle AddOnLoginStatusChangedDelegate_Handle(int32 LocalUserNum, const FOnLoginStatusChangedDelegate& Delegate) override;
	virtual void ClearOnLoginStatusChangedDelegate_Handle(int32 LocalUserNum, FDelegateHandle& Handle) override;
	virtual void TriggerOnLoginStatusChangedDelegates(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId) override;

	//OnControllerPairingChanged
	virtual FDelegateHandle AddOnControllerPairingChangedDelegate_Handle(const FOnControllerPairingChangedDelegate& Delegate) override;
	virtual void ClearOnControllerPairingChangedDelegate_Handle(FDelegateHandle& Handle) override;
	virtual void TriggerOnControllerPairingChangedDelegates(int LocalUserNum, FControllerPairingChangedUserInfo PreviousUser, FControllerPairingChangedUserInfo NewUser) override;

	//OnLoginComplete
	virtual FDelegateHandle AddOnLoginCompleteDelegate_Handle(int32 LocalUserNum, const FOnLoginCompleteDelegate& Delegate) override;
	virtual void ClearOnLoginCompleteDelegate_Handle(int32 LocalUserNum, FDelegateHandle& Handle) override;
	virtual void TriggerOnLoginCompleteDelegates(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error) override;

	//OnLogoutComplete
	virtual FDelegateHandle AddOnLogoutCompleteDelegate_Handle(int32 LocalUserNum, const FOnLogoutCompleteDelegate& Delegate) override;
	virtual void ClearOnLogoutCompleteDelegate_Handle(int32 LocalUserNum, FDelegateHandle& Handle) override;
	virtual void TriggerOnLogoutCompleteDelegates(int32 LocalUserNum, bool bWasSuccessful) override;

	//OnLoginFlowLogout
	virtual FDelegateHandle AddOnLoginFlowLogoutDelegate_Handle(const FOnLoginFlowLogoutDelegate& Delegate) override;
	virtual void ClearOnLoginFlowLogoutDelegate_Handle(FDelegateHandle& Handle) override;
	virtual void TriggerOnLoginFlowLogoutDelegates(const TArray<FString>& LoginDomains) override;
};

typedef TSharedPtr<class FOnlineIdentityPlayFab, ESPMode::ThreadSafe> FOnlineIdentityPlayFabPtr;