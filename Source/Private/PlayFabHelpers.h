#pragma once

bool
MakePlayFabRestRequest(
	const FString& TitleId,
	const FString& EntityToken,
	const FString& UrlPath,
	const FString& JsonPayload,
	const FHttpRequestCompleteDelegate& RequestCompleteCallback
);

FString SerializeRequestJson(
	TSharedPtr<FJsonObject> RequestJsonObject
);

void
GenerateGetTitlePlayersFromXboxLiveIDsRequestBody(
	const TArray<FString> Xuids,
	const FString Sandbox,
	FString& generatedRequestBody
	);

bool
GenerateUserAttributes(
	const FString& AttributeName,
	const FString& AttributeValue,
	FString& serializedAttributesJson
	);

bool ParseGetTitlePlayersFromXboxLiveIDsResponse(
	FHttpResponsePtr HttpResponse,
	TMap<FString, FString>& EntityIdMapping
	);