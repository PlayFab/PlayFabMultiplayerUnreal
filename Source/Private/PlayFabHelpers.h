#pragma once

namespace API
{
	// The name of the token parameter expected by the PlayFab login API.
#if defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
	const FString AuthTokenLabel("XboxToken");
#elif defined(OSS_PLAYFAB_SWITCH)
	const FString AuthTokenLabel("IdentityToken");
#elif defined(OSS_PLAYFAB_WIN64)
	const FString AuthTokenLabel("SteamTicket");
#endif // defined(OSS_PLAYFAB_WINGDK) || defined(OSS_PLAYFAB_XSX) || defined(OSS_PLAYFAB_XBOXONEGDK)
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

#if OSS_PLAYFAB_PLAYSTATION
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