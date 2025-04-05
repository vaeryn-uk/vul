#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * Utilities for working with UEnum definitions.
 */
namespace VulRuntime::Enum
{
	/**
	 * Returns all the enum values of a UEnum.
	 */
	template <typename EnumType>
	TArray<EnumType> Values()
	{
		TArray<EnumType> Out;

		// -1 as we exclude EnumType::MAX.
		for (int I = 0; I < StaticEnum<EnumType>()->NumEnums() - 1; I++)
		{
			Out.Add(static_cast<EnumType>(StaticEnum<EnumType>()->GetValueByIndex(I)));
		}

		return Out;
	}

	/**
	 * Returns string values for all values in an enum.
	 *
	 * The enum type must have an EnumToString implementation.
	 */
	template <typename EnumType>
	TArray<FString> StringValues()
	{
		TArray<FString> Out;

		for (const auto Val : Values<EnumType>())
		{
			Out.Add(EnumToString(Val));
		}
		
		return Out;
	}

	/**
	 * Finds the enum value corresponding to the given string.
	 *
	 * Returns false if it could not be found.
	 */
	template <typename EnumType>
	bool FromString(const FString& Str, EnumType& Out, const ESearchCase::Type MatchType = ESearchCase::IgnoreCase)
	{
		for (const auto Val : Values<EnumType>())
		{
			if (EnumToString(Val).Equals(Str, MatchType))
			{
				Out = Val;
				return true;
			}
		}
		
		return false;
	}

	/**
	 * An alternative to UE's EnumToString DECLARE macros with improved performance via a simple cache.
	 *
	 * Not thread safe.
	 */
	template <typename EnumType>
	FString EnumToString(const EnumType Value)
	{
		static TMap<EnumType, FString> Cache;
		if (const FString* Cached = Cache.Find(Value))
		{
			return *Cached;
		}
		
		const UEnum* EnumPtr = StaticEnum<EnumType>();
		
		FString Name = EnumPtr ? EnumPtr->GetNameStringByValue(static_cast<int64>(Value)) : TEXT("Invalid");
		Cache.Add(Value, Name);
		
		return Name;
	}
}
