//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

THIRD_PARTY_INCLUDES_START
#include <PFEntityKey.h>
THIRD_PARTY_INCLUDES_END

/* forward declaration */
class FOnlineSubsystemPlayFab;

struct EventContents
{
	PFEntityKey Entity;
	FString EventNamespace;
	FString Name;
	FString OriginalId;
	FDateTime OriginalTimestamp;
	TSharedPtr<FJsonObject> Payload;

	TSharedPtr<FJsonObject> Serialize() const;
};

struct WriteEventContents
{
	TArray<EventContents> Events;

	TSharedPtr<FJsonObject> Serialize() const;
};

class PlayFabEventTracer
{
PACKAGE_SCOPE:
	class FOnlineSubsystemPlayFab* OSSPlayFab = nullptr;
public:
	PlayFabEventTracer(FOnlineSubsystemPlayFab* InOSSPlayFab);

	void OnPlayFabMultiplayerManagerInitialize();
	void DoWork();

private:
	bool LoggedEntity = false;
	WriteEventContents EventsRequest;

	FString GetPlatformSDKVersion() const;
	void SetCommonTelemetryProperties(TSharedPtr<FJsonObject> Payload);
};
