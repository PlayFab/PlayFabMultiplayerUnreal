//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#if defined(OSS_PLAYFAB_GDK)
#include "OnlineExternalUIInterfacePlayFab.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "GDKTaskQueueHelpers.h"
#include "GDKUserManager.h"
#include "OnlineSessionInterfacePlayFab.h"

#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include <XGameUI.h>
#include <xsapi-c/multiplayer_activity_c.h>
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"

bool FOnlineExternalUIPlayFab::ShowInviteUI(int32 InLocalUserNum, FName InSessionName)
{
	UE_LOG_ONLINE_EXTERNALUI(Verbose, TEXT("FOnlineExternalUIPlayFab::ShowInviteUI()"));

	XAsyncBlock* pNewAsyncBlock = new XAsyncBlock();
	ZeroMemory(pNewAsyncBlock, sizeof(*pNewAsyncBlock));
	pNewAsyncBlock->context = this;
	pNewAsyncBlock->queue = FGDKAsyncTaskQueue::GetGenericQueue();

	pNewAsyncBlock->callback = [](XAsyncBlock* async)
	{
		bool bSuccess = false;

		TUniquePtr<XAsyncBlock> AsyncBlock(async);
		if (FOnlineExternalUIPlayFab* pThis = static_cast<FOnlineExternalUIPlayFab*>(AsyncBlock->context))
		{
			HRESULT Hr = XGameUiShowMultiplayerActivityGameInviteResult(async);
			if (SUCCEEDED(Hr))
			{
				bSuccess = true;
			}
			else
			{
				UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("FOnlineSessionPlayFab::HandleReadFriendsComplete: XAsyncGetStatus: result 0x%08x"), Hr);
			}
			
			pThis->HandleShowInviteUIComplete(bSuccess);
		}
	};

	FGDKUserHandle RequestedUser = IGDKRuntimeModule::Get().GetUserHandleByPlatformId(FPlatformMisc::GetPlatformUserForUserIndex(InLocalUserNum));
	HRESULT Hr = XGameUiShowMultiplayerActivityGameInviteAsync(pNewAsyncBlock, RequestedUser);
	if (FAILED(Hr))
	{
		UE_LOG_ONLINE_EXTERNALUI(Warning, TEXT("FOnlineExternalUIPlayFab::HandleReadFriendsComplete: Couldn't show system player picker UI for InLocalUserNum %d."), InLocalUserNum);
		delete pNewAsyncBlock;

		HandleShowInviteUIComplete(false);
	}

	return true;
}
#endif // OSS_PLAYFAB_GDK