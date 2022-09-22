//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef OSS_PLAYFAB_PLAYSTATION
#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"

class FOnlineSubsystemPlayFab;

class FOnlineAsyncTaskPlayFab : public FOnlineAsyncTaskBasic<FOnlineSubsystemPlayFab>
{
public:
	/**
	 * Constructor
	 * @param Subsystem the owning subsystem
	 */
	FOnlineAsyncTaskPlayFab(FOnlineSubsystemPlayFab* const Subsystem)
		: FOnlineAsyncTaskBasic(Subsystem)
	{
	}

	/** Destructor */
	virtual ~FOnlineAsyncTaskPlayFab() = default;

private:
	/** Default constructor disabled */
	FOnlineAsyncTaskPlayFab() = delete;
};

class FOnlineAsyncTaskManagerPlayFab : public FOnlineAsyncTaskManager
{
protected:
	/** reference to the online subsystem */
	FOnlineSubsystemPlayFab* const OSSPlayFab;

public:
	/**
	 * Constructor
	 * @param Subsystem the owning subsystem
	 */
	FOnlineAsyncTaskManagerPlayFab(FOnlineSubsystemPlayFab* InOnlineSubsystem)
		: OSSPlayFab(InOnlineSubsystem)
	{
	}

	/** Destructor */
	virtual ~FOnlineAsyncTaskManagerPlayFab()
	{
	}

	//~ Begin FOnlineAsyncTaskManager
	virtual void OnlineTick() override;
	//~ End FOnlineAsyncTaskManager

	/**
	 * Check if the current thread is the online thread
	 * @see IsOnGameThread
	 * @return true if we are on the online thread, false if not
	 */
	bool IsInOnlineThread() const;
};
#endif // OSS_PLAYFAB_PLAYSTATION
