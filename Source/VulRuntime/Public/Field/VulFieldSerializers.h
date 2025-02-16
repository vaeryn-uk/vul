#pragma once
#include "FVulFieldSerializationContext.h"

template <typename T>
struct FVulFieldSerializer
{
	static bool Serialize(const T& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		static_assert(sizeof(T) == -1, "Error: " __FUNCSIG__ " is not defined.");
		return nullptr;
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out, FVulFieldDeserializationContext& Ctx)
	{
		static_assert(sizeof(T) == -1, "Error: " __FUNCSIG__ " is not defined.");
		return false;
	}
};

template<>
struct FVulFieldSerializer<bool>
{
	static bool Serialize(const bool& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueBoolean>(Value);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, bool& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.Errors.RequireJsonType(Data, EJson::Boolean))
		{
			return false;
		}

		return Ctx.Errors.AddIfNot(Data->TryGetBool(Out), TEXT("serialized data is not a bool"));
	}
};

template<>
struct FVulFieldSerializer<int>
{
	static bool Serialize(const int& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueNumber>(Value);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, int& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.Errors.RequireJsonType(Data, EJson::Number))
		{
			return false;
		}

		return Ctx.Errors.AddIfNot(Data->TryGetNumber(Out), TEXT("serialized data is not a number"));
	}
};

template<>
struct FVulFieldSerializer<FString>
{
	static bool Serialize(const FString& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueString>(Value);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, FString& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.Errors.RequireJsonType(Data, EJson::String))
		{
			return false;
		}

		return Ctx.Errors.AddIfNot(Data->TryGetString(Out), TEXT("serialized data is not a string"));
	}
};
