#include "PlayFabHelpers.h"

#include "GenericPlatform/GenericPlatformMisc.h"

bool
MakePlayFabRestRequest(
	const FString& TitleId,
	const FString& EntityToken,
	const FString& UrlPath, 
	const TArray<TPair<FString, FString>>& ExtraHeaders,
	const FString& JsonPayload, 
	const FHttpRequestCompleteDelegate& RequestCompleteCallback
	)
{

	FHttpRequestRef httpRequest = FHttpModule::Get().CreateRequest();
	httpRequest->OnProcessRequestComplete() = RequestCompleteCallback;

	FString fullUrl = FString::Printf(TEXT("https://%s.playfabapi.com%s"), *TitleId, *UrlPath);
	httpRequest->SetHeader(TEXT("X-EntityToken"), *EntityToken);
	httpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	for (const auto& extraHeader : ExtraHeaders)
	{
		httpRequest->SetHeader(extraHeader.Key, extraHeader.Value);
	}
	httpRequest->SetURL(fullUrl);
	httpRequest->SetVerb(TEXT("POST"));
	httpRequest->SetContentAsString(JsonPayload); // gets copied internally
	httpRequest->ProcessRequest();

	return true;
}

FString SerializeRequestJson(
	TSharedPtr<FJsonObject> RequestJsonObject
	)
{
	// Serialize request body
	FString RequestBodySerialized;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBodySerialized);
	FJsonSerializer::Serialize(RequestJsonObject.ToSharedRef(), Writer);
	return RequestBodySerialized;
}

void
GenerateGetTitlePlayersFromXboxLiveIDsRequestBody(
	const TArray<FString>& Xuids,
	const FString Sandbox,
	FString& generatedRequestBody
)
{
	TSharedPtr<FJsonObject> HttpRequestJson = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> XuidsJson;
	for (const auto& Xuid : Xuids)
	{
		XuidsJson.Add(MakeShared<FJsonValueString>(Xuid));
	}

	HttpRequestJson->SetArrayField(TEXT("XboxLiveIds"), XuidsJson);
	HttpRequestJson->SetStringField(TEXT("Sandbox"), Sandbox);
	generatedRequestBody = SerializeRequestJson(HttpRequestJson);
}

void
GenerateGetPlayFabIDsFromSteamIDsRequestBody(
	const TArray<FString>& SteamUniqueIDs,
	FString& generatedRequestBody
)
{
	TSharedPtr<FJsonObject> HttpRequestJson = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> steamIDsJson;
	for (const auto& steamID : SteamUniqueIDs)
	{
		steamIDsJson.Add(MakeShared<FJsonValueString>(steamID));
	}

	HttpRequestJson->SetArrayField(TEXT("SteamStringIDs"), steamIDsJson);
	generatedRequestBody = SerializeRequestJson(HttpRequestJson);
}

void
GenerateGetPlayFabIDsFromNsaIDsRequestBody(
	const TArray<FString>& NsaUniqueIDs,
	FString& generatedRequestBody
)
{
	TSharedPtr<FJsonObject> HttpRequestJson = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> NsaIDsJson;
	for (const auto& NsaID : NsaUniqueIDs)
	{
		NsaIDsJson.Add(MakeShared<FJsonValueString>(NsaID));
	}

	HttpRequestJson->SetArrayField(TEXT("NintendoAccountIDs"), NsaIDsJson);
	generatedRequestBody = SerializeRequestJson(HttpRequestJson);
}

#if OSS_PLAYFAB_PLAYSTATION
void
GenerateGetPlayFabIDsFromPsnIDsRequestBody(
	const TArray<FString>& PsnUniqueIDs,
	FString& generatedRequestBody
)
{
	TSharedPtr<FJsonObject> HttpRequestJson = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> PsnIDsJson;
	for (const auto& PsnID : PsnUniqueIDs)
	{
		PsnIDsJson.Add(MakeShared<FJsonValueString>(PsnID));
	}

	HttpRequestJson->SetArrayField(TEXT("PSNAccountIDs"), PsnIDsJson);
	int32 NpIssuerIdValue = 0;
#if !UE_BUILD_SHIPPING
	if (!GConfig->GetInt(TEXT("OnlineSubsystemPlayFab"), TEXT("SceNpIssuerId"), NpIssuerIdValue, GGameIni))
	{
		UE_LOG_ONLINE(Error, TEXT("GenerateGetPlayFabIDsFromPsnIDsRequestBody: Failed to load SceNpIssuerId from GGameIni"));
	}
#endif // !UE_BUILD_SHIPPING
	HttpRequestJson->SetNumberField(FString(TEXT("IssuerId")), NpIssuerIdValue);
	generatedRequestBody = SerializeRequestJson(HttpRequestJson);
}
#endif // OSS_PLAYFAB_PLAYSTATION

bool
GenerateUserAttributes(
	const FString& AttributeName,
	const FString& AttributeValue,
	FString& serializedAttributesJson
	)
{
	TSharedPtr<FJsonObject> HttpRequestJson = MakeShareable(new FJsonObject());
	HttpRequestJson->SetStringField(AttributeName, AttributeValue);

	serializedAttributesJson = SerializeRequestJson(HttpRequestJson);
	return true;
}

bool ParseDataObjectFromPlayFabResponse(
	const FHttpResponsePtr HttpResponse,
	TSharedRef<TJsonReader<TCHAR>>& JsonReader,
	TSharedPtr<FJsonObject>& HttpResponseJSON,
	const TSharedPtr<FJsonObject>*& OutDataObject
)
{
	if (!HttpResponse.IsValid())
	{
		return false;
	}

	if (HttpResponse->GetResponseCode() != 200)
	{
		const FString ErrorResponseStr = HttpResponse->GetContentAsString();
		UE_LOG_ONLINE(Error, TEXT("ParseDataObjectFromPlayFabResponse failed with response code %u, response:%s!"), HttpResponse->GetResponseCode(), *ErrorResponseStr);
		return false;
	}

	if (!FJsonSerializer::Deserialize(JsonReader, HttpResponseJSON))
	{
		return false;
	}

	if (!HttpResponseJSON->TryGetObjectField(TEXT("data"), OutDataObject))
	{
		return false;
	}

	return true;
}

bool ParseSteamIdToPlayFabIdMappingDataObject(
	const TSharedPtr<FJsonObject>* JsonDataObject,
	TMap<FString, FString>& EntityIdMapping
)
{
//{
//		"code": 200,
//		"status" : "OK",
//		"data" : {
//		"Data": [
//		{
//			"SteamId": 123456789012345678,
//			"SteamStringId" : "123456789012345678",
//			"PlayFabId" : "9876543210987654321"
//		}
//		]
//	}
//}
	const TArray<TSharedPtr<FJsonValue>>* JsonIDPairArrayObject = nullptr;
	if (!(*JsonDataObject)->TryGetArrayField(TEXT("Data"), JsonIDPairArrayObject))
	{
		return false;
	}

	for (const auto& Value : *JsonIDPairArrayObject)
	{
		if (Value.IsValid())
		{
			FString SteamId;
			if (!(Value->AsObject())->TryGetStringField(TEXT("SteamId"), SteamId))
			{
				continue;
			}
			FString PlayFabId;
			if (!(Value->AsObject())->TryGetStringField(TEXT("PlayFabId"), PlayFabId))
			{
				continue;
			}
			EntityIdMapping.Add(SteamId, PlayFabId);
		}
	}
	return true;
}

bool
ParseNsaIdToPlayFabIdMappingDataObject(
	const TSharedPtr<FJsonObject>* JsonDataObject,
	TMap<FString, FString>& EntityIdMapping
)
{
	//{
	//		"code": 200,
	//		"status" : "OK",
	//		"data" : {
	//		"Data": [
	//		{
	//			"NintendoServiceAccountId": 12ba56789012345678,
	//			"PlayFabId" : "9876543210987654321"
	//		}
	//		]
	//	}
	//}
	const TArray<TSharedPtr<FJsonValue>>* JsonIDPairArrayObject = nullptr;
	if (!(*JsonDataObject)->TryGetArrayField(TEXT("Data"), JsonIDPairArrayObject))
	{
		return false;
	}

	for (const auto& Value : *JsonIDPairArrayObject)
	{
		if (Value.IsValid())
		{
			FString NsaId;
			if (!(Value->AsObject())->TryGetStringField(TEXT("NintendoServiceAccountId"), NsaId))
			{
				continue;
			}
			FString PlayFabId;
			if (!(Value->AsObject())->TryGetStringField(TEXT("PlayFabId"), PlayFabId))
			{
				continue;
			}
			EntityIdMapping.Add(NsaId, PlayFabId);
		}
	}
	return true;
}

#if OSS_PLAYFAB_PLAYSTATION
bool
ParsePsnIdToPlayFabIdMappingDataObject(
	const TSharedPtr<FJsonObject>* JsonDataObject,
	TMap<FString, FString>& EntityIdMapping
)
{
	//{
	//		"code": 200,
	//		"status" : "OK",
	//		"data" : {
	//		"Data": [
	//		{
	//			"PSNAccountId": ???,
	//			"PlayFabId" : "9876543210987654321"
	//		}
	//		]
	//	}
	//}
	const TArray<TSharedPtr<FJsonValue>>* JsonIDPairArrayObject = nullptr;
	if (!(*JsonDataObject)->TryGetArrayField(TEXT("Data"), JsonIDPairArrayObject))
	{
		return false;
	}

	for (const auto& Value : *JsonIDPairArrayObject)
	{
		if (Value.IsValid())
		{
			FString PsnId;
			if (!(Value->AsObject())->TryGetStringField(TEXT("PSNAccountId"), PsnId))
			{
				continue;
			}
			FString PlayFabId;
			if (!(Value->AsObject())->TryGetStringField(TEXT("PlayFabId"), PlayFabId))
			{
				continue;
			}
			EntityIdMapping.Add(PsnId, PlayFabId);
		}
	}
	return true;
}
#endif // OSS_PLAYFAB_PLAYSTATION

bool ParsePlayFabIdsFromPlatformIdsResponse(
	const FHttpResponsePtr HttpResponse,
	TMap<FString, FString>& EntityIdMapping
	)
{
	const FString ResponseStr = HttpResponse->GetContentAsString();
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseStr);
	TSharedPtr<FJsonObject> HttpResponseJSON;
	const TSharedPtr<FJsonObject>* JsonDataObject;
	if (!ParseDataObjectFromPlayFabResponse(HttpResponse, JsonReader, HttpResponseJSON, JsonDataObject))
	{
		return false;
	}

#ifdef OSS_PLAYFAB_WIN64	
	return ParseSteamIdToPlayFabIdMappingDataObject(JsonDataObject, EntityIdMapping);
#elif defined(OSS_PLAYFAB_SWITCH)
	return ParseNsaIdToPlayFabIdMappingDataObject(JsonDataObject, EntityIdMapping);
#elif defined(OSS_PLAYFAB_PLAYSTATION)
	return ParsePsnIdToPlayFabIdMappingDataObject(JsonDataObject, EntityIdMapping);
#endif
	return false;
}

bool ParseTitleAccountIDsFromPlatformIDsResponse(
	const FHttpResponsePtr HttpResponse,
	TMap<FString, FString>& EntityIdMapping
	)
{
// {
//	"code" : 200,
//	"status" : "OK",
//	"data" : {
//	"TitlePlayerAccounts" : 
//		{
//		"1234567890123456" : null,
//		"1234567890654321" : 
//			{
//				"NamespaceId" : "1234567890123456",
//				"TitleId" : "ABCD",
//				"MasterPlayerAccountId" : "1234567890123456",
//				"TitlePlayerAccountId" : "1234567890123456"
//			}
//		}
//	}
//}

	const FString ResponseStr = HttpResponse->GetContentAsString();
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseStr);
	TSharedPtr<FJsonObject> HttpResponseJSON;
	const TSharedPtr<FJsonObject>* JsonDataObject;
	if (!ParseDataObjectFromPlayFabResponse(HttpResponse, JsonReader, HttpResponseJSON, JsonDataObject))
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* JsonTitlePlayerAccounts = nullptr;
	if (!(*JsonDataObject)->TryGetObjectField(TEXT("TitlePlayerAccounts"), JsonTitlePlayerAccounts))
	{
		return false;
	}

	for (auto ValueIter = (*JsonTitlePlayerAccounts)->Values.CreateConstIterator(); ValueIter; ++ValueIter)
	{
		const FString Xuid = (*ValueIter).Key;
		TSharedPtr< FJsonValue > Value = (*ValueIter).Value;
		if (Value.IsValid())
		{
			FString EntityId;
			if (!(Value->AsObject())->TryGetStringField(TEXT("TitlePlayerAccountId"), EntityId))
			{
				continue;
			}
			EntityIdMapping.Add(Xuid, EntityId);
		}
	}
	return true;
}

bool ParseTitlePlayerAccountFromPlayerCombinedInfoResponse(
	const FHttpResponsePtr HttpResponse,
	FString& OutTitlePlayerAccountId
	)
{
/*
{
	"code": 200,
	"status": "OK",
	"data": {
		"PlayFabId": "<ID>",
		"InfoResultPayload": {
			"AccountInfo": {
				"PlayFabId": "<ID>",
				"Created": "2021-10-31T23:19:04.731Z",
				"TitleInfo": {
					"Created": "2021-10-31T23:19:04.731Z",
					"TitlePlayerAccount": {
						"Id": "<ID>",
						"Type": "title_player_account",
						"TypeString": "title_player_account"
					}
				}
			},
			"UserDataVersion": 0,
			"UserReadOnlyDataVersion": 0
		}
	}
}
*/
	const FString ResponseStr = HttpResponse->GetContentAsString();
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseStr);
	TSharedPtr<FJsonObject> HttpResponseJSON;
	const TSharedPtr<FJsonObject>* JsonDataObject;
	if (!ParseDataObjectFromPlayFabResponse(HttpResponse, JsonReader, HttpResponseJSON, JsonDataObject))
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* JsonObject = nullptr;
	if (!(*JsonDataObject)->TryGetObjectField(TEXT("InfoResultPayload"), JsonObject)
		|| !(*JsonObject)->TryGetObjectField(TEXT("AccountInfo"), JsonObject)
		|| !(*JsonObject)->TryGetObjectField(TEXT("TitleInfo"), JsonObject)
		|| !(*JsonObject)->TryGetObjectField(TEXT("TitlePlayerAccount"), JsonObject)
		|| !(*JsonObject)->TryGetStringField(TEXT("Id"), OutTitlePlayerAccountId))
	{
		return false;
	}

	return true;
}

void ParseDeviceMakeModel(FString& DeviceMake, FString& DeviceModel)
{
	// "DeviceMake|DeviceModel" if possible, and "CPUVendor|CPUBrand" otherwise
	// optionally returns "DeviceMake|DeviceModel|CPUChipset" if known
	// We wont try to read CPUChipset
	const FString DeviceMakeAndModel = FGenericPlatformMisc::GetDeviceMakeAndModel();

	const int32 FirstIdx = DeviceMakeAndModel.Find(TEXT("|"), ESearchCase::IgnoreCase, ESearchDir::Type::FromStart, 0);
	if (FirstIdx == INDEX_NONE)
	{
		DeviceMake = TEXT("Unknown");
		DeviceModel = TEXT("Unknown");
		return;
	}
	DeviceMake = DeviceMakeAndModel.Mid(0 /* start */, FirstIdx);

	const int32 SecondIdx = DeviceMakeAndModel.Find(TEXT("|"), ESearchCase::IgnoreCase, ESearchDir::Type::FromStart, FirstIdx + 1);
	const int32 EndSecondWord = SecondIdx == INDEX_NONE ? DeviceMakeAndModel.Len() : SecondIdx;
	DeviceModel = DeviceMakeAndModel.Mid(FirstIdx + 1, (EndSecondWord - FirstIdx) - 1);
}