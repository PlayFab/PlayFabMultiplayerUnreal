//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "OnlineExternalUIInterfacePlayFab.h"
#include "OnlineSubsystemPlayFabPrivate.h"
#include "OnlineSessionInterfacePlayFab.h"
#include "OnlineSubsystemPlayFab.h"
#include "OnlineIdentityInterfacePlayFab.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CoreDelegates.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeLock.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineFriendsInterface.h"

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineDelegateMacros.h"

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include <XGameUI.h>
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"
#endif

const int32 PEOPLE_PICKER_MAX_SIZE = 100;
#define INVITE_UI_TEXT TEXT("Invite players")

#define OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE IOnlineSubsystem* NativeSubsystem = IOnlineSubsystem::GetByPlatform();  IOnlineExternalUIPtr NativeExternalUIInterface = NativeSubsystem ? NativeSubsystem->GetExternalUIInterface() : nullptr; if (NativeExternalUIInterface)

bool FOnlineExternalUIPlayFab::ShowLoginUI(const int32 ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowLoginUI(ControllerIndex, bShowOnlineOnly, bShowSkipButton, Delegate);
	}
	return false;
}

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
void FOnlineExternalUIPlayFab::HandleShowLoginUIComplete(bool bSuccess, FGDKUserHandle GDKUser, FOnLoginUIClosedDelegate Delegate)
{
	// TODO: investigate if this is needed:
	OSSPlayFab->ExecuteNextTick([this, GDKUser, Delegate]()
		{
		});
}
#endif

bool FOnlineExternalUIPlayFab::ShowFriendsUI(int32 LocalUserNum)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowFriendsUI(LocalUserNum);
	}

	return false;
}

bool FOnlineExternalUIPlayFab::ShowInviteUI(int32 LocalUserNum, FName SessionNameParam)
{
	const IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	check(IdentityIntPtr.IsValid());

	TSharedPtr<const FUniqueNetId> UniqueId = IdentityIntPtr->GetUniquePlayerId(LocalUserNum);

	if (!UniqueId.IsValid())
	{
		UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("ShowInviteUI: Couldn't find valid user for LocalUserNum %d."), LocalUserNum);
		return false;
	}

	FOnlineSessionPlayFabPtr PlayFabSession = StaticCastSharedPtr<FOnlineSessionPlayFab>(OSSPlayFab->GetSessionInterface());
	FNamedOnlineSessionPtr Session = PlayFabSession->GetNamedSessionPtr(SessionNameParam);
	if (!Session.IsValid())
	{
		UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("ShowInviteUI: Named session not found for %s session name. Can't send invite."), *SessionNameParam.ToString());
		return false;
	}

	this->SessionName = SessionNameParam;
	FOnlineSessionInfoPlayFabPtr PlayFabSessionInfo = StaticCastSharedPtr<FOnlineSessionInfoPlayFab>(Session->SessionInfo);
	if (!PlayFabSessionInfo->IsValid())
	{
		UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("ShowInviteUI: FOnlineSessionInfoPlayFab not valid for %s. Can't send invite."), *SessionNameParam.ToString());
		return false;
	}

	IOnlineFriendsPtr FriendsIntPtr = OSSPlayFab->GetFriendsInterface();
	check(FriendsIntPtr.IsValid());
	bool ReadFriendsListSuccess = FriendsIntPtr->ReadFriendsList(
		LocalUserNum,
		EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateRaw(this, &FOnlineExternalUIPlayFab::HandleReadFriendsComplete)
	);
	if (!ReadFriendsListSuccess)
	{
		UE_LOG_ONLINE_EXTERNALUI(Error, TEXT("ShowInviteUI: Couldn't read friends list for LocalUserNum %d."), LocalUserNum);
		return false;
	}

	return true;
}

void FOnlineExternalUIPlayFab::HandleReadFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
#if defined(OSS_PLAYFAB_WIN64) || defined(OSS_PLAYFAB_SWITCH)
	UE_LOG_ONLINE_EXTERNALUI(Error, TEXT("HandleReadFriendsCompleted is not implemented for this platform."));
	return;
#elif defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	if (!bWasSuccessful)
	{
		UE_LOG_ONLINE_EXTERNALUI(Error, TEXT("HandleReadFriendsComplete: Couldn't read friends list for LocalUserNum %d."), LocalUserNum);
		return;
	}

	IOnlineFriendsPtr FriendsIntPtr = OSSPlayFab->GetFriendsInterface();
	check(FriendsIntPtr.IsValid());
	TArray<TSharedRef<FOnlineFriend>> FriendsList;
	bool GetFriendListSuccess = FriendsIntPtr->GetFriendsList(LocalUserNum, EFriendsLists::ToString(EFriendsLists::Default), FriendsList);
	if (!GetFriendListSuccess)
	{
		UE_LOG_ONLINE_EXTERNALUI(Error, TEXT("HandleReadFriendsComplete: Couldn't get friends list for LocalUserNum %d."), LocalUserNum);
		return;
	}

	auto FriendsCount = FriendsList.Num();
	if (FriendsCount > 0)
	{
		unsigned playerIndex = 0;
		for (const TSharedRef<FOnlineFriend>& FriendPtr: FriendsList)
		{
			this->InviteUISelectFromPlayers[playerIndex] = StaticCastSharedRef<const FUniqueNetIdGDK>(FriendPtr->GetUserId())->ToUint64();
			this->InviteUIPreSelectedPlayers[playerIndex] = StaticCastSharedRef<const FUniqueNetIdGDK>(FriendPtr->GetUserId())->ToUint64();
			playerIndex++;
		}
	}

	// Show friend selection UI
	XAsyncBlock* pNewAsyncBlock = new XAsyncBlock();
	ZeroMemory(pNewAsyncBlock, sizeof(*pNewAsyncBlock));
	pNewAsyncBlock->context = this;
	pNewAsyncBlock->queue = FGDKAsyncTaskQueue::GetGenericQueue();

	pNewAsyncBlock->callback = [](XAsyncBlock* ab)
	{
		TUniquePtr<XAsyncBlock> AsyncBlock(ab);
		FOnlineExternalUIPlayFab* Owner = static_cast<FOnlineExternalUIPlayFab*>(AsyncBlock->context);
		Owner->ProcessShowPlayerPickerResults(MoveTemp(AsyncBlock));
	};

	//Store the local user
	LocalUserNumber = LocalUserNum;

	FGDKUserHandle RequestedUser = FGDKUserManager::Get().GetUserHandleByPlatformId(LocalUserNum);
	HRESULT ErrorCode = XGameUiShowPlayerPickerAsync(
		pNewAsyncBlock,
		RequestedUser,
		"Please, select players to invite:",
		FriendsCount,
		this->InviteUISelectFromPlayers,
		0,
		this->InviteUIPreSelectedPlayers,
		0,
		FriendsCount
	);

	if (ErrorCode != S_OK)
	{
		UE_LOG_ONLINE_EXTERNALUI(Error, TEXT("HandleReadFriendsComplete: Couldn't show system player picker UI for LocalUserNum %d."), LocalUserNum);
		delete pNewAsyncBlock;
		return;
	}
#endif
}

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
void FOnlineExternalUIPlayFab::ProcessShowPlayerPickerResults(TUniquePtr<XAsyncBlock> AsyncBlock)
{
	uint32_t ResultPlayersCount = 0;
	TArray<uint64_t> InvitedResultPlayers;
	uint32_t InviteResultPlayersCount;
	HRESULT Hr = XGameUiShowPlayerPickerResultCount(AsyncBlock.Get(), &ResultPlayersCount);
	if (SUCCEEDED(Hr))
	{
		InvitedResultPlayers.SetNum(ResultPlayersCount);
		Hr = XGameUiShowPlayerPickerResult(AsyncBlock.Get(), ResultPlayersCount, InvitedResultPlayers.GetData(), &InviteResultPlayersCount);
		if (SUCCEEDED(Hr) && (InviteResultPlayersCount > 0))
		{
			IOnlineSessionPtr SessionIntPtr = OSSPlayFab->GetSessionInterface();
			check(SessionIntPtr.IsValid());

			TArray<TSharedRef<const FUniqueNetId>> SelectedFriends;
			for (uint32_t i = 0; i < InviteResultPlayersCount; i++)
			{
				SelectedFriends.Add(FUniqueNetIdGDK::Create(InvitedResultPlayers[i]));
			}

			bool SendSessionInviteSuccess = SessionIntPtr->SendSessionInviteToFriends(LocalUserNumber, SessionName, SelectedFriends);
			if (!SendSessionInviteSuccess)
			{
				UE_LOG_ONLINE_EXTERNALUI(Error, TEXT("FOnlineExternalUIPlayFab::ProcessShowPlayerPickerResults: Couldn't send a session invite to selected players for LocalUserNum %d."), LocalUserNumber);
			}
		}
		else
		{
			UE_LOG_ONLINE_EXTERNALUI(Error, TEXT("FOnlineExternalUIPlayFab::XGameUiShowPlayerPickerResult: failed with ErrorCode=[0x%08x]."), Hr);
		}
	}
	else
	{
		UE_LOG_ONLINE_EXTERNALUI(Error, TEXT("FOnlineExternalUIPlayFab::XGameUiShowPlayerPickerResultCount: failed with ErrorCode=[0x%08x]."), Hr);
	}
}
#endif

void FOnlineExternalUIPlayFab::HandleShowSendGameInvitesUIComplete(bool bIsSuccess)
{
	// TODO: investigate if this is needed:
	OSSPlayFab->ExecuteNextTick([this, bIsSuccess]()
		{
			TriggerOnExternalUIChangeDelegates(bIsSuccess);
		});
}

bool FOnlineExternalUIPlayFab::ShowAchievementsUI(int32 LocalUserNum)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowAchievementsUI(LocalUserNum);
	}

	return true;
}

void FOnlineExternalUIPlayFab::HandleShowAchievementsUIComplete(bool bIsSuccess)
{
	if (bIsSuccess)
	{
		UE_LOG_ONLINE_EXTERNALUI(Log, TEXT("ShowAchievementsUI: Achievement task UI displaying."));
	}
	else
	{
		UE_LOG_ONLINE_EXTERNALUI(Log, TEXT("ShowAchievementsUI: Achievement task UI failed. Not displaying."));
	}
}

bool FOnlineExternalUIPlayFab::ShowLeaderboardUI(const FString& LeaderboardName)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowLeaderboardUI(LeaderboardName);
	}

	return false;
}

bool FOnlineExternalUIPlayFab::ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowWebURL(Url, ShowParams, Delegate);
	}

	return true;
}

FOnlineExternalUIPlayFab::FOnlineExternalUIPlayFab(FOnlineSubsystemPlayFab* InSubsystem)
	: OSSPlayFab(InSubsystem)
{
}

bool FOnlineExternalUIPlayFab::CloseWebURL()
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->CloseWebURL();
	}

	return false;
}

bool FOnlineExternalUIPlayFab::ShowAccountUpgradeUI(const FUniqueNetId& UniqueId)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowAccountUpgradeUI(UniqueId);
	}

	return false;
}

bool FOnlineExternalUIPlayFab::ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowStoreUI(LocalUserNum, ShowParams, Delegate);
	}

	return true;
}

void FOnlineExternalUIPlayFab::HandleShowStoreUIComplete(bool wasPurchaseMade)
{
	// TODO: investigate if this is needed
}

bool FOnlineExternalUIPlayFab::ShowSendMessageUI(int32 LocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowSendMessageUI(LocalUserNum, ShowParams, Delegate);
	}

	return false;
}

bool FOnlineExternalUIPlayFab::ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate)
{
	OSS_PLAYFAB_GET_NATIVE_EXTERNALUI_INTERFACE
	{
		return NativeExternalUIInterface->ShowProfileUI(Requestor, Requestee, Delegate);
	}

	return true;
}

void FOnlineExternalUIPlayFab::HandleShowProfileUIComplete(bool bSuccess, const FOnProfileUIClosedDelegate Delegate)
{
	// TODO: investigate if this is needed
}

void FOnlineExternalUIPlayFab::Tick(float DeltaTime)
{
	// TODO: investigate if this is needed
}

