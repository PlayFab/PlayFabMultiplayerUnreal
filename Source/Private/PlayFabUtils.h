#pragma once

class UTF8StringList
{
public:
	void Add(const FString& InString)
	{
		TArray<UTF8CHAR>& UTF8KeyArray = UTF8Strings.AddDefaulted_GetRef();
		auto Convert = StringCast<UTF8CHAR>(*InString);
		UTF8KeyArray.Append(Convert.Get(), Convert.Length());
		UTF8KeyArray.Push(UTF8CHAR(0));
		UTF8StringPtrs.Add(UTF8KeyArray.GetData());
		Count += 1;
	}

	void AddNull()
	{
		UTF8StringPtrs.Add(nullptr);
		Count += 1;
	}

	const char** GetData()
	{
		return (const char**)UTF8StringPtrs.GetData();
	}

	const uint32_t GetCount()
	{
		return Count;
	}
private:
	uint32_t Count{ 0 };
	TArray<TArray<UTF8CHAR>> UTF8Strings;
	TArray<const UTF8CHAR*> UTF8StringPtrs;
};