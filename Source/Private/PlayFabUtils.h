#pragma once

class AnsiStringList
{
public:
	void Add(const FString& InString)
	{
		TArray<ANSICHAR>& AnsiKeyArray = AnsiStrings.AddDefaulted_GetRef();
		AnsiKeyArray.Append(TCHAR_TO_UTF8(*InString), InString.Len() + 1);
		AnsiStringPtrs.Add(AnsiKeyArray.GetData());
		Count += 1;
	}

	void AddNull()
	{
		AnsiStringPtrs.Add(nullptr);
		Count += 1;
	}

	const char** GetData()
	{
		return AnsiStringPtrs.GetData();
	}

	const uint32_t GetCount()
	{
		return Count;
	}
private:
	uint32_t Count{ 0 };
	TArray<TArray<ANSICHAR>> AnsiStrings;
	TArray<const ANSICHAR*> AnsiStringPtrs;
};