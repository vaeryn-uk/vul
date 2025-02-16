﻿#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

struct FVulFieldSerializationErrors
{
	bool IsSuccess() const;
	
	template <typename FmtType, typename... Types>
	void Add(const FmtType& Fmt, Types&&... Args)
	{
		Errors.Add(FString::Printf(Fmt, Forward<Types>(Args)...));
	}
	
	template <typename FmtType, typename... Types>
	bool AddIfNot(const bool Condition, const FmtType& Fmt, Types&&... Args)
	{
		if (!Condition)
		{
			Errors.Add(FString::Printf(Fmt, Forward<Types>(Args)...));
		}

		return true;
	}

	/**
	 * Convenience function to check that the provided value is of type, returning false
	 * and adding an error if not.
	 */
	bool RequireJsonType(const TSharedPtr<FJsonValue>& Value, const EJson Type);

	TArray<FString> Errors;
};

struct FVulFieldSerializationContext
{
	FVulFieldSerializationErrors Errors;
};

struct FVulFieldDeserializationContext
{
	FVulFieldSerializationErrors Errors;
};

FString JsonTypeToString(EJson Type)
{
	switch (Type)
	{
	case EJson::None:     return TEXT("None");
	case EJson::Null:     return TEXT("Null");
	case EJson::String:   return TEXT("String");
	case EJson::Number:   return TEXT("Number");
	case EJson::Boolean:  return TEXT("Boolean");
	case EJson::Array:    return TEXT("Array");
	case EJson::Object:   return TEXT("Object");
	default:              return TEXT("Unknown");
	}
}

