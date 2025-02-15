#pragma once

template <typename T>
struct FVulFieldSerializer
{
	static TSharedPtr<FJsonValue> Serialize(const T&)
	{
		static_assert(sizeof(T) == -1, "Error: " __FUNCSIG__ " is not defined.");
		return nullptr;
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>&, T&)
	{
		static_assert(sizeof(T) == -1, "Error: " __FUNCSIG__ " is not defined.");
		return false;
	}
};

template<>
struct FVulFieldSerializer<bool>
{
	static TSharedPtr<FJsonValue> Serialize(const bool& Value) { return MakeShared<FJsonValueBoolean>(Value); }
	static bool Deserialize(const TSharedPtr<FJsonValue>& Json, bool& OutValue)
	{
		return Json->Type == EJson::Boolean && Json->TryGetBool(OutValue);
	}
};

template<>
struct FVulFieldSerializer<int>
{
	static TSharedPtr<FJsonValue> Serialize(const int& Value) { return MakeShared<FJsonValueNumber>(Value); }
	static bool Deserialize(const TSharedPtr<FJsonValue>& Json, int& OutValue)
	{
		return Json->Type == EJson::Number && Json->TryGetNumber(OutValue);
	}
};

template<>
struct FVulFieldSerializer<FString>
{
	static TSharedPtr<FJsonValue> Serialize(const FString& Value) { return MakeShared<FJsonValueString>(Value); }
	static bool Deserialize(const TSharedPtr<FJsonValue>& Json, FString& OutValue)
	{ 
		return Json->Type == EJson::String && Json->TryGetString(OutValue);
	}
};
