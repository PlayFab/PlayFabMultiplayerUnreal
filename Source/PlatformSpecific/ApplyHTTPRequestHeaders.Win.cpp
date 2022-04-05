#if OSS_PLAYFAB_WIN64
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
    TSharedPtr<FJsonObject> RequestBodyJson;
    if (NativeSubsystem)
    {
        IOnlineIdentityPtr NativeIdentityInterface = NativeSubsystem->GetIdentityInterface();
        if (NativeIdentityInterface && NativeIdentityInterface.IsValid())
        {
            FString AuthToken = NativeIdentityInterface->GetAuthToken(0);

            RequestBodyJson = MakeShareable(new FJsonObject());
            RequestBodyJson->SetStringField(TEXT("SteamTicket"), AuthToken);

            UserAuthRequestData MetaData;
            UserAuthRequestsInFlight.Add(PlatformUserID, MetaData);

            headersApplied = true;
        }
    }
    FOnlineIdentityPlayFab::OnPopulatePlatformRequestDataCompleted(headersApplied, PlatformUserID, TMap<FString, FString>(), RequestBodyJson);
    return headersApplied;
}
#endif