//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "OnlineIdentityInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"

#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonTypes.h"
#include "Serialization/JsonSerializer.h"

#include "HttpModule.h"
#include "OnlineSubsystem.h"

#define OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();  IOnlineIdentityPtr NativeIdentityInterface = NativeSubsystem ? NativeSubsystem->GetIdentityInterface() : nullptr; if (NativeIdentityInterface)

static constexpr float AuthCoolDownTime = 3.0f;

FOnlineIdentityPlayFab::FOnlineIdentityPlayFab(class FOnlineSubsystemPlayFab* InSubsystem) : 
	OSSPlayFab(InSubsystem),
	TimeSinceLastAuth(AuthCoolDownTime) // Don't wait on the cool down to auth on the very first go
{
	check(OSSPlayFab);
}

FOnlineIdentityPlayFab::~FOnlineIdentityPlayFab()
{
	OSSPlayFab = nullptr;

	CleanUpAuthDelegates();
	CleanUpLocalUsers();
}

bool FOnlineIdentityPlayFab::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::Login"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->Login(LocalUserNum, AccountCredentials);
	}
	
	return false;
}

bool FOnlineIdentityPlayFab::Logout(int32 LocalUserNum)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::Logout"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->Logout(LocalUserNum);
	}

	return false;
}

bool FOnlineIdentityPlayFab::AutoLogin(int32 LocalUserNum)
{
	return Login(LocalUserNum, FOnlineAccountCredentials());
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityPlayFab::GetUserAccount(const FUniqueNetId& UserId) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetUserAccount"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetUserAccount(UserId);
	}

	return nullptr;
}

TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityPlayFab::GetAllUserAccounts() const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetAllUserAccounts"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetAllUserAccounts();
	}

	return TArray<TSharedPtr<FUserOnlineAccount>>();
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityPlayFab::GetUniquePlayerId(int32 LocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetUniquePlayerId"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetUniquePlayerId(LocalUserNum);
	}

	return nullptr;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityPlayFab::GetSponsorUniquePlayerId(int32 LocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetSponsorUniquePlayerId"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetSponsorUniquePlayerId(LocalUserNum);
	}

	return nullptr;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityPlayFab::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::CreateUniquePlayerId"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->CreateUniquePlayerId(Bytes, Size);
	}

	return nullptr;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityPlayFab::CreateUniquePlayerId(const FString& Str)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::CreateUniquePlayerId"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->CreateUniquePlayerId(Str);
	}

	return nullptr;
}

ELoginStatus::Type FOnlineIdentityPlayFab::GetLoginStatus(int32 LocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetLoginStatus"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetLoginStatus(LocalUserNum);
	}

	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityPlayFab::GetLoginStatus(const FUniqueNetId& UserId) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetLoginStatus"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetLoginStatus(UserId);
	}

	return ELoginStatus::NotLoggedIn;
}

FString FOnlineIdentityPlayFab::GetPlayerNickname(int32 LocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetPlayerNickname"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetPlayerNickname(LocalUserNum);
	}

	return TEXT("");
}

FString FOnlineIdentityPlayFab::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetPlayerNickname"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetPlayerNickname(UserId);
	}

	return TEXT("");
}

FString FOnlineIdentityPlayFab::GetAuthToken(int32 LocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetAuthToken"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetAuthToken(LocalUserNum);
	}

	return TEXT("");
}

void FOnlineIdentityPlayFab::RevokeAuthToken(const FUniqueNetId& UserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::RevokeAuthToken"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		NativeIdentityInterface->RevokeAuthToken(UserId, Delegate);
	}
}

void FOnlineIdentityPlayFab::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetUserPrivilege"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		NativeIdentityInterface->GetUserPrivilege(UserId, Privilege, Delegate);
	}
}

FPlatformUserId FOnlineIdentityPlayFab::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetPlatformUserIdFromUniqueNetId"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetPlatformUserIdFromUniqueNetId(UniqueNetId);
	}

	return PLATFORMUSERID_NONE;
}

FString FOnlineIdentityPlayFab::GetAuthType() const
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::GetAuthType"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->GetAuthType();
	}

	return TEXT("PlayFabParty");
}

void FOnlineIdentityPlayFab::Tick(float DeltaTime)
{
	TimeSinceLastAuth += DeltaTime;
	
	// Auth all players once at start or after resume
	if (bAuthAllUsers == true)
	{
		// The native identity interface may not be constructed yet
		OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
		{
			UE_LOG_ONLINE(Verbose, TEXT("FOnlineIdentityPlayFab::Tick: Native identity interface initialized, registering auth delegates and doing initial user auth"));

			if (bRegisterAuthDelegates)
			{
				RegisterAuthDelegates();
			}

			for (const auto& User : NativeIdentityInterface->GetAllUserAccounts())
			{
				FString PlatformUserIdStr;
				User->GetUserAttribute(USER_ATTR_ID, PlatformUserIdStr);
				UsersToAuth.Add(PlatformUserIdStr);
			}
#if defined(OSS_PLAYFAB_WIN64) || defined(OSS_PLAYFAB_PLAYSTATION)
			AutoLogin(0);
#endif
			bAuthAllUsers = false;
		}
	}

	// Don't bother trying to auth users while PlayFab is not initialized and ready
	if (OSSPlayFab && OSSPlayFab->bPartyInitialized)
	{
		TryAuthenticateUsers();
	}
}

void FOnlineIdentityPlayFab::OnAppSuspend()
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::OnAppSuspend"));
	
	CleanUpLocalUsers();
}

void FOnlineIdentityPlayFab::OnAppResume()
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::OnAppResume"));

	bAuthAllUsers = true;
}

void FOnlineIdentityPlayFab::RegisterAuthDelegates()
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::RegisterAuthDelegates"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		for (int32 i = 0; i < MAX_LOCAL_PLAYERS; ++i)
		{
			FDelegateHandle NewHandle = NativeIdentityInterface->AddOnLoginStatusChangedDelegate_Handle(i, FOnLoginStatusChangedDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnLoginStatusChanged));
			LoginStatusChangedDelegateHandles.Add(NewHandle);
		}

		for (int32 i = 0; i < MAX_LOCAL_PLAYERS; ++i)
		{
			FDelegateHandle NewHandle = NativeIdentityInterface->AddOnLoginCompleteDelegate_Handle(i, FOnLoginCompleteDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnLoginComplete));
			LoginCompleteDelegateHandles.Add(NewHandle);
		}

		for (int32 i = 0; i < MAX_LOCAL_PLAYERS; ++i)
		{
			FDelegateHandle NewHandle = NativeIdentityInterface->AddOnLogoutCompleteDelegate_Handle(i, FOnLogoutCompleteDelegate::CreateRaw(this, &FOnlineIdentityPlayFab::OnLogoutComplete));
			LogoutCompleteDelegateHandles.Add(NewHandle);
		}
	}

	bRegisterAuthDelegates = false;
}

void FOnlineIdentityPlayFab::CleanUpAuthDelegates()
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::CleanUpAuthDelegates"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		for (int32 i = 0; i < LoginStatusChangedDelegateHandles.Num(); ++i)
		{
			NativeIdentityInterface->ClearOnLoginStatusChangedDelegate_Handle(i, LoginStatusChangedDelegateHandles[i]);
		}

		for (int32 i = 0; i < LoginCompleteDelegateHandles.Num(); ++i)
		{
			NativeIdentityInterface->ClearOnLoginCompleteDelegate_Handle(i, LoginCompleteDelegateHandles[i]);
		}

		for (int32 i = 0; i < LogoutCompleteDelegateHandles.Num(); ++i)
		{
			NativeIdentityInterface->ClearOnLogoutCompleteDelegate_Handle(i, LogoutCompleteDelegateHandles[i]);
		}

		LoginStatusChangedDelegateHandles.Empty();
		LoginCompleteDelegateHandles.Empty();
		LogoutCompleteDelegateHandles.Empty();
	}

	bRegisterAuthDelegates = true;
}

void FOnlineIdentityPlayFab::OnLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::OnLoginStatusChanged"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		if (NewId.IsValid())
		{
			if (NewStatus == ELoginStatus::LoggedIn)
			{
				UsersToAuth.Add(NewId.ToString());
			}
			else if (NewStatus == ELoginStatus::NotLoggedIn)
			{
				// Clean up the PlayFab local user
				FString PlatformUserIdStr = NewId.ToString();
				RemoveLocalUser(PlatformUserIdStr);
			}
		}
	}
}

void FOnlineIdentityPlayFab::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::OnLoginComplete"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		if (bWasSuccessful && UserId.IsValid())
		{
			UsersToAuth.Add(UserId.ToString());
		}
	}
}

void FOnlineIdentityPlayFab::OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::OnLogoutComplete"));

	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		// Clean up the PlayFab local user
		if (bWasSuccessful)
		{
			TSharedPtr<const FUniqueNetId> UserId = NativeIdentityInterface->GetUniquePlayerId(LocalUserNum);
			if (UserId.IsValid())
			{
				FString PlatformUserIdStr = UserId->ToString();
				RemoveLocalUser(PlatformUserIdStr);
			}
		}
	}
}

void FOnlineIdentityPlayFab::TryAuthenticateUsers()
{
	// Update any users that need to update their tokens
	for (TSharedPtr<FPlayFabUser> LocalUser : LocalPlayFabUsers)
	{
		if (FDateTime::Now() > LocalUser->GetEntityTokenUpdateTime())
		{
			UsersToAuth.Add(LocalUser->GetPlatformUserId());
		}
	}
	
	// Only try to auth if we have waited long enough and have users that need it
	if (TimeSinceLastAuth > AuthCoolDownTime && UsersToAuth.Num() != 0)
	{
		for (const FString& PlatformUserIdStr : UsersToAuth)
		{
			AuthenticateUser(PlatformUserIdStr);
		}
	}
}

void FOnlineIdentityPlayFab::CleanUpLocalUsers()
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::CleanUpLocalUsers"));

	LocalPlayFabUsers.Empty();

	UsersToAuth.Empty();
	UserAuthRequestsInFlight.Empty();
}

bool FOnlineIdentityPlayFab::AuthenticateUser(const FString& PlatformUserIdStr)
{
	// Only if not already in flight for this user
	if (UserAuthRequestsInFlight.Contains(PlatformUserIdStr))
	{
		return false;
	}

	const bool bAppliedPlatformData = ApplyPlatformHTTPRequestData(PlatformUserIdStr, API::Url, API::PostVerb);

	if (bAppliedPlatformData && !UserAuthRequestsInFlight.Contains(PlatformUserIdStr))
	{
		UserAuthRequestData MetaData;
		UserAuthRequestsInFlight.Add(PlatformUserIdStr, MetaData);
	}

	return bAppliedPlatformData;
}

// Called after platform has appended its headers/body
void FOnlineIdentityPlayFab::FinishRequest(bool bPlatformDataSuccess, const FString& PlatformUserIdStr, TMap<FString, FString> PlatformHeaders, TSharedPtr<FJsonObject> RequestBodyJson)
{
	if (bPlatformDataSuccess)
	{
		// PlayFab auth request
		FHttpRequestPtr httpRequest = FHttpModule::Get().CreateRequest();

		UserAuthRequestData* RequestInFlight = GetUserAuthRequestData(PlatformUserIdStr);
		if (RequestInFlight != nullptr)
		{
			RequestInFlight->m_HTTPRequest = httpRequest;

			RequestInFlight->m_HTTPRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineIdentityPlayFab::Auth_HttpRequestComplete);
			FString TitleIdStr = OSSPlayFab->GetAppId();

			// Add all platform headers
			for (const auto& kvPair : PlatformHeaders)
			{
				RequestInFlight->m_HTTPRequest->SetHeader(kvPair.Key, kvPair.Value);
			}

			// Build up the rest of the request body
			RequestBodyJson->SetBoolField(TEXT("CreateAccount"), true);
			RequestBodyJson->SetStringField(TEXT("TitleId"), TitleIdStr);

			// Serialize request body
			FString RequestBodySerialized;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBodySerialized);
			FJsonSerializer::Serialize(RequestBodyJson.ToSharedRef(), Writer);

#ifdef OSS_PLAYFAB_SWITCH
			const FString LoginApi = "LoginWithNintendoServiceAccount";
#elif defined(OSS_PLAYFAB_WIN64)
			const FString LoginApi = "LoginWithSteam";
#elif defined(OSS_PLAYFAB_PLAYSTATION)
			const FString LoginApi = "LoginWithPSN";
#else
			const FString LoginApi = "LoginWithXbox";
#endif // OSS_PLAYFAB_SWITCH

			const FString URI = FString::Printf(TEXT("https://%s.playfabapi.com/Client/%s"), *TitleIdStr, *LoginApi);

			RequestInFlight->m_HTTPRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
			RequestInFlight->m_HTTPRequest->SetURL(URI);
			RequestInFlight->m_HTTPRequest->SetVerb(API::PostVerb);
			RequestInFlight->m_HTTPRequest->SetContentAsString(RequestBodySerialized); // gets copied internally
			RequestInFlight->m_HTTPRequest->ProcessRequest();
		}
	}
	else
	{
		// Remove the in flight data
		UserAuthRequestsInFlight.Remove(PlatformUserIdStr);
	}
}

void FOnlineIdentityPlayFab::Auth_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	const FString& PlatformUserIdStr = GetUserPlatformIdStrFromRequest(HttpRequest);

	FString ResponseStr;
	FString ErrorStr;

	if (bSucceeded && HttpResponse.IsValid())
	{		
		ResponseStr = HttpResponse->GetContentAsString();

		// Deserialize response
		TSharedPtr<FJsonObject> HttpResponseJSON = nullptr;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseStr);
		if (FJsonSerializer::Deserialize(JsonReader, HttpResponseJSON))
		{
			const TSharedPtr<FJsonObject>* JsonData = nullptr;
			if (HttpResponseJSON->TryGetObjectField(TEXT("data"), JsonData))
			{
				const TSharedPtr<FJsonObject>* JsonEntityTokenData = nullptr;
				if ((*JsonData)->TryGetObjectField(TEXT("EntityToken"), JsonEntityTokenData))
				{
					FString EntityTokenStr;
					FString TokenExpirationStr;

					if ((*JsonEntityTokenData)->TryGetStringField(TEXT("EntityToken"), EntityTokenStr) && (*JsonEntityTokenData)->TryGetStringField(TEXT("TokenExpiration"), TokenExpirationStr))
					{
						const TSharedPtr<FJsonObject>* JsonEntity = nullptr;
						if ((*JsonEntityTokenData)->TryGetObjectField(TEXT("Entity"), JsonEntity))
						{
							FString EntityIdStr, EntityTypeStr;
							if ((*JsonEntity)->TryGetStringField(TEXT("Id"), EntityIdStr) && (*JsonEntity)->TryGetStringField(TEXT("Type"), EntityTypeStr))
							{
								UE_LOG_ONLINE(Verbose, TEXT("[FOnlineIdentityPlayFab::Auth_HttpRequestComplete] Platform User %s authenticated with entityid %s and token %s and expiration %s"), *PlatformUserIdStr, *EntityIdStr, *EntityTokenStr, *TokenExpirationStr);
								
								TSharedPtr<FPlayFabUser> LocalUser = GetPartyLocalUserFromPlatformIdString(PlatformUserIdStr);
								if (LocalUser)
								{
									// Update an existing user if we already have one
									LocalUser->UpdateEntityToken(EntityTokenStr);
								}
								else
								{
									// Obtain the SessionTicket from the PlayFab auth response for use in any subsequent requests demanding the X-Authorization header.
									FString SessionTicketStr = "";
									if ((*JsonData)->TryGetStringField(TEXT("SessionTicket"), SessionTicketStr) == false)
									{
										UE_LOG_ONLINE(Error, TEXT("[FOnlineIdentityPlayFab::Auth_HttpRequestComplete] Failed to parse SessionTicket data"));
									}

									// Create a new user if we don't already have one
									CreateLocalUser(PlatformUserIdStr, EntityIdStr, EntityTypeStr, SessionTicketStr, EntityTokenStr, TokenExpirationStr);
								}

								TimeSinceLastAuth = 0.0f;
								UsersToAuth.Remove(PlatformUserIdStr);
							}
							else
							{
								// MS_ATG_PNF: in error cases, the user will re-auth on the next attempt
								UE_LOG_ONLINE(Error, TEXT("[FOnlineIdentityPlayFab::Auth_HttpRequestComplete] Failed to parse EntityId data"));
							}
						}
						else
						{
							// MS_ATG_PNF: in error cases, the user will re-auth on the next attempt
							UE_LOG_ONLINE(Error, TEXT("[FOnlineIdentityPlayFab::Auth_HttpRequestComplete] Failed to parse Entity data"));
						}
					}
					else
					{
						// MS_ATG_PNF: in error cases, the user will re-auth on the next attempt
						UE_LOG_ONLINE(Error, TEXT("[FOnlineIdentityPlayFab::Auth_HttpRequestComplete] Failed to parse EntityToken data"));
					}
				}
				else
				{
					// MS_ATG_PNF: in error cases, the user will re-auth on the next attempt
					UE_LOG_ONLINE(Error, TEXT("[FOnlineIdentityPlayFab::Auth_HttpRequestComplete] Failed to parse EntityToken data"));
				}
			}
			else
			{
				// MS_ATG_PNF: in error cases, the user will re-auth on the next attempt
				UE_LOG_ONLINE(Error, TEXT("[FOnlineIdentityPlayFab::Auth_HttpRequestComplete] Failed to parse JSON data"));
			}
		}
		else
		{
			// MS_ATG_PNF: in error cases, the user will re-auth on the next attempt
			UE_LOG_ONLINE(Error, TEXT("[FOnlineIdentityPlayFab::Auth_HttpRequestComplete] Failed to deserialize response"));
		}
	}

	// Remove the in flight data
	UserAuthRequestsInFlight.Remove(PlatformUserIdStr);
}

void FOnlineIdentityPlayFab::CreateLocalUser(const FString& PlatformUserIdStr, const FString& EntityId, const FString& EntityType, const FString& SessionTicket, const FString& EntityToken, const FString& TokenExpiration)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::CreateLocalUser"));

	PartyLocalUser* NewPartyLocalUser = nullptr;
	PartyError Err;

	// Create a local user object
	Err = PartyManager::GetSingleton().CreateLocalUser(
		TCHAR_TO_UTF8(*EntityId),		// User id
		TCHAR_TO_UTF8(*EntityToken),	// User entity token
		&NewPartyLocalUser				// OUT local user object
	);

	if (PARTY_FAILED(Err))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab PartyManager::CreateLocalUser failed: %s"), *GetPartyErrorMessage(Err));
		return;
	}
	
	TSharedPtr<FPlayFabUser> NewLocalUser = MakeShared<FPlayFabUser>(PlatformUserIdStr, EntityToken, EntityId, EntityType, SessionTicket, NewPartyLocalUser);

	PFEntityKey EntityKey = NewLocalUser->GetEntityKey();
	HRESULT hr = PFMultiplayerSetEntityToken(
		OSSPlayFab->GetMultiplayerHandle(), // Multiplayer Handle 
		&EntityKey,							// EntityKey
		TCHAR_TO_UTF8(*EntityToken)  		// User entity token
	);

	if (FAILED(hr))
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab MultiplayerManager::GetSingleton().SetEntityToken failed: 0x%x"), hr);
		PartyManager::GetSingleton().DestroyLocalUser(NewPartyLocalUser, nullptr);
		return;
	}
	LocalPlayFabUsers.Add(NewLocalUser);

	// Listening to invite is best effort
	OSSPlayFab->GetPlayFabLobbyInterface()->RegisterInvitationListener(EntityKey);
}

void FOnlineIdentityPlayFab::RemoveLocalUser(const FString& PlatformUserIdStr)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::RemoveLocalUser"));

	if (PlatformUserIdStr.IsEmpty() == false)
	{
		UserAuthRequestsInFlight.Remove(PlatformUserIdStr);
		UsersToAuth.Remove(PlatformUserIdStr);

		for (int32 i = 0; i < LocalPlayFabUsers.Num(); ++i)
		{
			TSharedPtr<FPlayFabUser> LocalUser = LocalPlayFabUsers[i];
			if (LocalUser->GetPlatformUserId().Compare(PlatformUserIdStr, ESearchCase::IgnoreCase) == 0)
			{
				PartyError Err = PartyManager::GetSingleton().DestroyLocalUser(LocalUser->GetPartyLocalUser(), nullptr);
				if (PARTY_FAILED(Err))
				{
					UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemPlayFab::DestroyLocalUser failed: %s"), *GetPartyErrorMessage(Err));
				}
				else
				{
					LocalPlayFabUsers.Remove(LocalUser);
				}

				// Stop Listening to invite is best effort
				OSSPlayFab->GetPlayFabLobbyInterface()->UnregisterInvitationListener(LocalUser->GetEntityKey());

				break;
			}
		}
	}
}

TSharedPtr<FPlayFabUser> FOnlineIdentityPlayFab::GetPartyLocalUserFromPlatformId(const FUniqueNetId& PlatformNetId)
{
	return GetPartyLocalUserFromPlatformIdString(PlatformNetId.ToString());
}

TSharedPtr<FPlayFabUser> FOnlineIdentityPlayFab::GetPartyLocalUserFromPlatformIdString(const FString& PlatformNetIdStr)
{
	if (PlatformNetIdStr.IsEmpty() == false)
	{
		for (TSharedPtr<FPlayFabUser> LocalUser : LocalPlayFabUsers)
		{
			if (LocalUser->GetPlatformUserId().Compare(PlatformNetIdStr, ESearchCase::IgnoreCase) == 0)
			{
				return LocalUser;
			}
		}
	}

	return nullptr;
}

TSharedPtr<FPlayFabUser> FOnlineIdentityPlayFab::GetPartyLocalUserFromEntityIdString(const FString& EntityIdString)
{
	if (EntityIdString.IsEmpty() == false)
	{
		for (TSharedPtr<FPlayFabUser> LocalUser : LocalPlayFabUsers)
		{
			if (LocalUser->GetEntityId().Compare(EntityIdString, ESearchCase::IgnoreCase) == 0)
			{
				return LocalUser;
			}
		}
	}

	return nullptr;
}

bool FOnlineIdentityPlayFab::IsUserLocal(const PFEntityKey& UserEntityKey)
{
	for (TSharedPtr<FPlayFabUser> LocalUser : LocalPlayFabUsers)
	{
		if ((FCStringAnsi::Strcmp(LocalUser->GetEntityKey().id, UserEntityKey.id) == 0) && (FCStringAnsi::Strcmp(LocalUser->GetEntityKey().type, UserEntityKey.type) == 0))
		{
			return true;
		}
	}
	return false;
}

FDelegateHandle FOnlineIdentityPlayFab::AddOnLoginChangedDelegate_Handle(const FOnLoginChangedDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->AddOnLoginChangedDelegate_Handle(Delegate);
	}

	return FDelegateHandle();
}

void FOnlineIdentityPlayFab::ClearOnLoginChangedDelegate_Handle(FDelegateHandle& Handle)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->ClearOnLoginChangedDelegate_Handle(Handle);
	}
}

void FOnlineIdentityPlayFab::TriggerOnLoginChangedDelegates(int32 LocalUserNum)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->TriggerOnLoginChangedDelegates(LocalUserNum);
	}
}

FDelegateHandle FOnlineIdentityPlayFab::AddOnLoginStatusChangedDelegate_Handle(int32 LocalUserNum, const FOnLoginStatusChangedDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->AddOnLoginStatusChangedDelegate_Handle(LocalUserNum, Delegate);
	}

	return FDelegateHandle();
}

void FOnlineIdentityPlayFab::ClearOnLoginStatusChangedDelegate_Handle(int32 LocalUserNum, FDelegateHandle& Handle)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->ClearOnLoginStatusChangedDelegate_Handle(LocalUserNum, Handle);
	}
}

void FOnlineIdentityPlayFab::TriggerOnLoginStatusChangedDelegates(int32 LocalUserNum, ELoginStatus::Type OldStatus, ELoginStatus::Type NewStatus, const FUniqueNetId& NewId)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->TriggerOnLoginStatusChangedDelegates(LocalUserNum, OldStatus, NewStatus, NewId);
	}
}

FDelegateHandle FOnlineIdentityPlayFab::AddOnControllerPairingChangedDelegate_Handle(const FOnControllerPairingChangedDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->AddOnControllerPairingChangedDelegate_Handle(Delegate);
	}

	return FDelegateHandle();
}

void FOnlineIdentityPlayFab::ClearOnControllerPairingChangedDelegate_Handle(FDelegateHandle& Handle)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->ClearOnControllerPairingChangedDelegate_Handle(Handle);
	}
}

void FOnlineIdentityPlayFab::TriggerOnControllerPairingChangedDelegates(int LocalUserNum, FControllerPairingChangedUserInfo PreviousUser, FControllerPairingChangedUserInfo NewUser)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->TriggerOnControllerPairingChangedDelegates(LocalUserNum, PreviousUser, NewUser);
	}
}

FDelegateHandle FOnlineIdentityPlayFab::AddOnLoginCompleteDelegate_Handle(int32 LocalUserNum, const FOnLoginCompleteDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->AddOnLoginCompleteDelegate_Handle(LocalUserNum, Delegate);
	}

	return FDelegateHandle();
}

void FOnlineIdentityPlayFab::ClearOnLoginCompleteDelegate_Handle(int32 LocalUserNum, FDelegateHandle& Handle)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, Handle);
	}
}

void FOnlineIdentityPlayFab::TriggerOnLoginCompleteDelegates(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->TriggerOnLoginCompleteDelegates(LocalUserNum, bWasSuccessful, UserId, Error);
	}
}

FDelegateHandle FOnlineIdentityPlayFab::AddOnLogoutCompleteDelegate_Handle(int32 LocalUserNum, const FOnLogoutCompleteDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->AddOnLogoutCompleteDelegate_Handle(LocalUserNum, Delegate);
	}

	return FDelegateHandle();
}

void FOnlineIdentityPlayFab::ClearOnLogoutCompleteDelegate_Handle(int32 LocalUserNum, FDelegateHandle& Handle)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->ClearOnLogoutCompleteDelegate_Handle(LocalUserNum, Handle);
	}
}

void FOnlineIdentityPlayFab::TriggerOnLogoutCompleteDelegates(int32 LocalUserNum, bool bWasSuccessful)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->TriggerOnLogoutCompleteDelegates(LocalUserNum, bWasSuccessful);
	}
}

FDelegateHandle FOnlineIdentityPlayFab::AddOnLoginFlowLogoutDelegate_Handle(const FOnLoginFlowLogoutDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->AddOnLoginFlowLogoutDelegate_Handle(Delegate);
	}

	return FDelegateHandle();
}

void FOnlineIdentityPlayFab::ClearOnLoginFlowLogoutDelegate_Handle(FDelegateHandle& Handle)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->ClearOnLoginFlowLogoutDelegate_Handle(Handle);
	}
}

void FOnlineIdentityPlayFab::TriggerOnLoginFlowLogoutDelegates(const TArray<FString>& LoginDomains)
{
	OSS_PLAYFAB_GET_NATIVE_IDENTITY_INTERFACE
	{
		return NativeIdentityInterface->TriggerOnLoginFlowLogoutDelegates(LoginDomains);
	}
}

PartyLocalUser* FOnlineIdentityPlayFab::GetFirstPartyLocalUser()
{
	if (LocalPlayFabUsers.Num() > 0)
	{
		return LocalPlayFabUsers[0]->GetPartyLocalUser();
	}

	return nullptr;
}

void FPlayFabUser::UpdateEntityToken(const FString& NewEntityToken)
{
	UE_LOG_ONLINE_IDENTITY(Verbose, TEXT("FOnlineIdentityPlayFab::UpdateEntityToken"));

	if (NewEntityToken.IsEmpty() == false)
	{
		if (LocalUser)
		{
			PartyError Err = LocalUser->UpdateEntityToken(TCHAR_TO_UTF8(*NewEntityToken));
			if (PARTY_FAILED(Err))
			{
				UE_LOG_ONLINE(Warning, TEXT("FPlayFabPartyUser::UpdateEntityToken failed: %s"), *GetPartyErrorMessage(Err));
			}
			else
			{
				EntityToken = NewEntityToken;
				SetNewEntityTokenUpdateTime();
			}
		}
	}
}

void FPlayFabUser::SetNewEntityTokenUpdateTime()
{
	EntityTokenUpdateTime = FDateTime::Now() + FTimespan::FromHours(12.0f);
}