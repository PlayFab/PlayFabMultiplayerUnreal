#pragma once
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Interfaces/IHttpResponse.h"
#include "OnlineSubsystemUtils.h"

namespace API
{
	// The name of the token parameter expected by the PlayFab login API.
#if defined(OSS_PLAYFAB_GDK)
	const FString AuthTokenLabel("XboxToken");
#elif defined(OSS_PLAYFAB_SWITCH)
	const FString AuthTokenLabel("IdentityToken");
#elif defined(OSS_PLAYFAB_WIN64)
	const FString AuthTokenLabel("SteamTicket");
#endif
	const FString Url = "https://playfabapi.com/";
	const FString PostVerb = "POST";
}

bool
MakePlayFabRestRequest(
	const FString& TitleId,
	const FString& EntityToken,
	const FString& UrlPath,
	const TArray<TPair<FString, FString>>& ExtraHeaders,
	const FString& JsonPayload,
	const FHttpRequestCompleteDelegate& RequestCompleteCallback
);

FString SerializeRequestJson(
	TSharedPtr<FJsonObject> RequestJsonObject
);

void
GenerateGetTitlePlayersFromXboxLiveIDsRequestBody(
	const TArray<FString>& Xuids,
	const FString Sandbox,
	FString& generatedRequestBody
	);

void
GenerateGetPlayFabIDsFromSteamIDsRequestBody(
	const TArray<FString>& SteamUniqueIDs,
	FString& generatedRequestBody
	);

void
GenerateGetPlayFabIDsFromNsaIDsRequestBody(
	const TArray<FString>& NsaUniqueIDs,
	FString& generatedRequestBody
);

#if defined(OSS_PLAYFAB_PLAYSTATION)
void
GenerateGetPlayFabIDsFromPsnIDsRequestBody(
	const TArray<FString>& PsnUniqueIDs,
	FString& generatedRequestBody
);
#endif // OSS_PLAYFAB_PLAYSTATION

bool
GenerateUserAttributes(
	const FString& AttributeName,
	const FString& AttributeValue,
	FString& serializedAttributesJson
	);

bool ParsePlayFabIdsFromPlatformIdsResponse(
	const FHttpResponsePtr HttpResponse,
	TMap<FString, FString>& EntityIdMapping
	);

bool ParseTitleAccountIDsFromPlatformIDsResponse(
	const FHttpResponsePtr HttpResponse,
	TMap<FString, FString>& EntityIdMapping
	);

bool ParseTitleAccountIDsFromPlatformIDsResponse(
	const FHttpResponsePtr HttpResponse,
	TMap<FString, FString>& EntityIdMapping
	);

bool ParseTitlePlayerAccountFromPlayerCombinedInfoResponse(
	const FHttpResponsePtr HttpResponse,
	FString& OutTitlePlayerAccountId
	);

void ParseDeviceMakeModel(FString& DeviceMake, FString& DeviceModel);