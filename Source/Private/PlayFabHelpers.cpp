
bool
MakePlayFabRestRequest(
	const FString& TitleId,
	const FString& EntityToken,
	const FString& UrlPath, 
	const FString& JsonPayload, 
	const FHttpRequestCompleteDelegate& RequestCompleteCallback
    )
{

	FHttpRequestRef httpRequest = FHttpModule::Get().CreateRequest();
	httpRequest->OnProcessRequestComplete() = RequestCompleteCallback;

	FString fullUrl = FString::Printf(TEXT("https://%s.playfabapi.com%s"), *TitleId, *UrlPath);
	httpRequest->SetHeader(TEXT("X-EntityToken"), *EntityToken);
	httpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
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
	const TArray<FString> Xuids,
	const FString Sandbox,
	FString& generatedRequestBody
)
{
	TSharedPtr<FJsonObject> HttpRequestJson = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> XuidsJson;
	for (auto Xuid : Xuids)
	{
		XuidsJson.Add(MakeShared<FJsonValueString>(Xuid));
	}

	HttpRequestJson->SetArrayField(TEXT("XboxLiveIds"), XuidsJson);
	HttpRequestJson->SetStringField(TEXT("Sandbox"), Sandbox);
	generatedRequestBody = SerializeRequestJson(HttpRequestJson);
}

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

bool ParseGetTitlePlayersFromXboxLiveIDsResponse(
	FHttpResponsePtr HttpResponse,
	TMap<FString, FString>& EntityIdMapping
)
{
	//{
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

	if (!HttpResponse.IsValid())
	{
		return false;
	}

	if (HttpResponse->GetResponseCode() != 200)
	{
		FString ErrorResponseStr = HttpResponse->GetContentAsString();
		UE_LOG_ONLINE(Error, TEXT("ParseGetTitlePlayersFromXboxLiveIDsResponse failed with response code %u, response:%s!"), HttpResponse->GetResponseCode(), *ErrorResponseStr);
		return false;
	}

	FString ResponseStr = HttpResponse->GetContentAsString();
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseStr);
	TSharedPtr<FJsonObject> HttpResponseJSON = nullptr;
	if (!FJsonSerializer::Deserialize(JsonReader, HttpResponseJSON))
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* JsonDataObject = nullptr;
	if (!HttpResponseJSON->TryGetObjectField(TEXT("data"), JsonDataObject))
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
