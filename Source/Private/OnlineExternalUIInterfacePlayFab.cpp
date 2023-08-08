#include "OnlineExternalUIInterfacePlayFab.h"

#define OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();  IOnlineExternalUIPtr NativeExternalUIInterface = NativeSubsystem ? NativeSubsystem->GetExternalUIInterface() : nullptr; if (NativeExternalUIInterface)

FOnlineExternalUIPlayFab::FOnlineExternalUIPlayFab(FOnlineSubsystemPlayFab* InSubsystem)
	: OSSPlayFab(InSubsystem)
{
	check(OSSPlayFab);
}

bool FOnlineExternalUIPlayFab::ShowLoginUI(const int32 ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowLoginUI()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowLoginUI(ControllerIndex, bShowOnlineOnly, bShowSkipButton, Delegate);
	}
	return false;
}

bool FOnlineExternalUIPlayFab::ShowFriendsUI(int32 InLocalUserNum)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowFriendsUI()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowFriendsUI(InLocalUserNum);
	}
	return false;
}

bool FOnlineExternalUIPlayFab::ShowAchievementsUI(int32 InLocalUserNum)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowAchievementsUI()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowAchievementsUI(InLocalUserNum);
	}
	return true;
}

bool FOnlineExternalUIPlayFab::ShowLeaderboardUI(const FString& LeaderboardName)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowLeaderboardUI()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowLeaderboardUI(LeaderboardName);
	}
	return false;
}

bool FOnlineExternalUIPlayFab::ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowWebURL()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowWebURL(Url, ShowParams, Delegate);
	}
	return true;
}

bool FOnlineExternalUIPlayFab::CloseWebURL()
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::CloseWebURL()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->CloseWebURL();
	}
	return false;
}

bool FOnlineExternalUIPlayFab::ShowAccountUpgradeUI(const FUniqueNetId& UniqueId)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowAccountUpgradeUI()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowAccountUpgradeUI(UniqueId);
	}
	return false;
}

bool FOnlineExternalUIPlayFab::ShowStoreUI(int32 InLocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowStoreUI()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowStoreUI(InLocalUserNum, ShowParams, Delegate);
	}
	return true;
}

bool FOnlineExternalUIPlayFab::ShowSendMessageUI(int32 InLocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowSendMessageUI()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowSendMessageUI(InLocalUserNum, ShowParams, Delegate);
	}
	return false;
}

bool FOnlineExternalUIPlayFab::ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowProfileUI()"));
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowProfileUI(Requestor, Requestee, Delegate);
	}
	return true;
}

void FOnlineExternalUIPlayFab::HandleShowInviteUIComplete(bool bIsSuccess)
{
	if (OSSPlayFab)
	{
		OSSPlayFab->ExecuteNextTick([this, bIsSuccess]()
			{
				TriggerOnExternalUIChangeDelegates(bIsSuccess);
			});
	}
}