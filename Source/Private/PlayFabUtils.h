#pragma once

class AnsiStringList
{
public:
	void Add(const FString& InString)
	{
		TArray<UTF8CHAR>& AnsiKeyArray = AnsiStrings.AddDefaulted_GetRef();
		auto Convert = StringCast<UTF8CHAR>(*InString);
		AnsiKeyArray.Append(Convert.Get(), Convert.Length());
		AnsiKeyArray.Push(UTF8CHAR(0));
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
		return (const char**)AnsiStringPtrs.GetData();
	}

	const uint32_t GetCount()
	{
		return Count;
	}
private:
	uint32_t Count{ 0 };
	TArray<TArray<UTF8CHAR>> AnsiStrings;
	TArray<const UTF8CHAR*> AnsiStringPtrs;
};
