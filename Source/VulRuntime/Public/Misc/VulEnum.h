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
}
