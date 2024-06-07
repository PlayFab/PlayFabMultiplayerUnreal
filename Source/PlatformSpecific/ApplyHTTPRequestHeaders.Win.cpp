//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#if defined(OSS_PLAYFAB_WIN64)
#include "OnlineSubsystem.h"
#include "OnlineIdentityInterfacePlayFab.h"

void FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(bool bWasSuccessful, const FString& PlatformUserID, TMap<FString, FString> PlatformHeaders, TSharedPtr<FJsonObject> RequestBodyJson)
{
    IOnlineSubsystem* OSSPlayFab = IOnlineSubsystem::Get(PLAYFAB_SUBSYSTEM);
    if (OSSPlayFab)
    {
        FOnlineIdentityPlayFabPtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityPlayFab>(OSSPlayFab->GetIdentityInterface());
        if (IdentityInterface.IsValid())
        {
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
    IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();
    bool headersApplied = false;
    if (NativeSubsystem)
    {
        IOnlineIdentityPtr NativeIdentityInterface = NativeSubsystem->GetIdentityInterface();
        if (NativeIdentityInterface && NativeIdentityInterface.IsValid())
        {
            FOnGetLinkedAccountAuthTokenCompleteDelegate Delegate = FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda(
                [this, InPlatformUserID = PlatformUserID](int32 LocalUserNum, bool bWasSuccessful, const FExternalAuthToken& AuthToken)
                {
                    if (bWasSuccessful && AuthToken.HasTokenString())
                    {
                        TSharedPtr<FJsonObject> RequestBodyJson;
                        RequestBodyJson = MakeShareable(new FJsonObject());
                        RequestBodyJson->SetStringField(TEXT("SteamTicket"), AuthToken.TokenString);
                        RequestBodyJson->SetBoolField(TEXT("TicketIsServiceSpecific"), true);

                        UserAuthRequestData MetaData;
                        UserAuthRequestsInFlight.Add(InPlatformUserID, MetaData);

                        FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(true, InPlatformUserID, TMap<FString, FString>(), RequestBodyJson);
                    }
                    else
                    {
                        FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(false, InPlatformUserID, TMap<FString, FString>(), nullptr);
                    }
            });
            NativeIdentityInterface->GetLinkedAccountAuthToken(0, TEXT("WebAPI:AzurePlayFab"), Delegate);
            headersApplied = true;
        }
    }
    return headersApplied;
}
#endif // OSS_PLAYFAB_WIN64