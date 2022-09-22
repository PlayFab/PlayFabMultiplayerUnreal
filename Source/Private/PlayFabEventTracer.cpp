//--------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "PlayFabEventTracer.h"
#include "PlayFabHelpers.h"

#include "GenericPlatform/GenericPlatformMisc.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "GenericPlatform/GenericPlatformProperties.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/NetworkVersion.h"

#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
#include "grdk.h"
#endif

static constexpr const char* EventNamespace = "playfab.party";
static constexpr const char* RequestPathTelemetry = "/Event/WriteTelemetryEvents";

TSharedPtr<FJsonObject> EventContents::Serialize() const
{
	TSharedPtr<FJsonObject> EventContentJson = MakeShareable(new FJsonObject());
	TSharedPtr<FJsonObject> EntityJson = MakeShareable(new FJsonObject());
	EntityJson->SetStringField(TEXT("Id"), Entity.id);
	EntityJson->SetStringField(TEXT("Type"), Entity.type);
	EventContentJson->SetObjectField(TEXT("Entity"), EntityJson);
	EventContentJson->SetStringField(TEXT("EventNamespace"), EventNamespace);
	EventContentJson->SetStringField(TEXT("Name"), Name);
	EventContentJson->SetStringField(TEXT("OriginalId"), OriginalId);
	EventContentJson->SetStringField(TEXT("OriginalTimestamp"), OriginalTimestamp.ToIso8601()); // conform with pf standards
	EventContentJson->SetObjectField(TEXT("Payload"), Payload);
	return EventContentJson;
}

TSharedPtr<FJsonObject> WriteEventContents::Serialize() const
{    
	TArray<TSharedPtr<FJsonValue>> EventsJson;
	for (const auto& Event : Events)
	{
		EventsJson.Emplace(MakeShared<FJsonValueObject>(Event.Serialize()));
	}

	TSharedPtr<FJsonObject> EventsJsonFinal = MakeShareable(new FJsonObject());
	EventsJsonFinal->SetArrayField(TEXT("Events"), EventsJson);
	return EventsJsonFinal;
}

PlayFabEventTracer::PlayFabEventTracer(FOnlineSubsystemPlayFab* InOSSPlayFab) :
	OSSPlayFab(InOSSPlayFab)
{}

void PlayFabEventTracer::OnPlayFabMultiplayerManagerInitialize()
{
	EventContents Event;
	Event.Name = TEXT("unreal_client_initialization_completed");
	Event.EventNamespace = FString(EventNamespace);
	Event.OriginalTimestamp = FDateTime::UtcNow();

	TSharedPtr<FJsonObject> PayloadJson = MakeShareable(new FJsonObject());
	SetCommonTelemetryProperties(PayloadJson); // Design for when more telemetry events are added

	PayloadJson->SetStringField(TEXT("ClientInstanceId"), FApp::GetSessionId().ToString());
	PayloadJson->SetStringField(TEXT("PartyVersion"), TEXT("1.7.8"));
	PayloadJson->SetStringField(TEXT("UnrealEngineVersion"), FEngineVersion::Current().ToString());

	PayloadJson->SetStringField(TEXT("PlatformSDKVersion"), GetPlatformSDKVersion());

	Event.Payload = PayloadJson;
	EventsRequest.Events.Add(Event);
}

void PlayFabEventTracer::DoWork()
{
	// Currently DoWork will be called only once since we are only logging one event.
	// Once more events will be added to telemetry, the structure of this function will change
	if (LoggedEntity == true)
	{
		return;
	}

	// error checking is done already for validity of GetIdentityInterface
	IOnlineIdentityPtr IdentityIntPtr = OSSPlayFab->GetIdentityInterface();
	FOnlineIdentityPlayFab* PlayFabIdentityInt = static_cast<FOnlineIdentityPlayFab*>(IdentityIntPtr.Get());
	if (PlayFabIdentityInt->GetFirstPartyLocalUser() == nullptr)
	{
		return;
	}
	LoggedEntity = true; // so that DoWork() is not called again
	TSharedPtr<FPlayFabUser> LocalUser = PlayFabIdentityInt->GetAllPartyLocalUsers()[0];
	EventsRequest.Events[0].Entity = LocalUser->GetEntityKey();

	TArray<TPair<FString, FString>> ExtraHeaders;
	ExtraHeaders.Add(MakeTuple(FString("X-Authorization"), LocalUser->GetSessionTicket()));

	FString GeneratedRequestBody = SerializeRequestJson(EventsRequest.Serialize());

	MakePlayFabRestRequest(OSSPlayFab->GetAppId(),
							LocalUser->GetEntityToken(),
							FString(RequestPathTelemetry),
							ExtraHeaders,
							GeneratedRequestBody,
							nullptr);
}

#if !defined(OSS_PLAYFAB_PLAYSTATION)
FString PlayFabEventTracer::GetPlatformSDKVersion() const
{
	#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	return FString::FromInt(_GRDK_EDITION);
	#elif defined(OSS_PLAYFAB_WIN64)
	//TODO: use STEAM_SDK_VER
	return "1.51";
	#elif defined(OSS_PLAYFAB_SWITCH)
	return "12.3.2";
	#else
	UE_LOG_ONLINE(Error, TEXT("Trying to retrieve SDK version for unsupported platform"));
	return "Unsupported Platform";
	#endif
}
#endif

void PlayFabEventTracer::SetCommonTelemetryProperties(TSharedPtr<FJsonObject> Payload)
{
	FString OSName , OSVersion;
	FPlatformMisc::GetOSVersions(OSName , OSVersion);
	Payload->SetStringField(TEXT("TitleID"), OSSPlayFab->GetAppId());
	Payload->SetStringField(TEXT("OSName"), OSName );
	Payload->SetStringField(TEXT("OSVersion"), OSVersion);

	FString DeviceMake, DeviceModel;
	ParseDeviceMakeModel(DeviceMake, DeviceModel);
	Payload->SetStringField(TEXT("DeviceMake"), DeviceMake);
	Payload->SetStringField(TEXT("DeviceModel"), DeviceModel);
	Payload->SetStringField(TEXT("AppName"), FString(FApp::GetProjectName()));
	Payload->SetStringField(TEXT("AppVersion"), FNetworkVersion::GetProjectVersion());
	#ifdef OSS_PLAYFAB_WIN64
	Payload->SetStringField(TEXT("PlatformName"), FString(FPlatformProperties::PlatformName()));
	#else
	Payload->SetStringField(TEXT("PlatformName"), FString(FGenericPlatformProperties::IniPlatformName()));
	#endif
}