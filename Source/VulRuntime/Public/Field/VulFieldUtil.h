#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

template <typename T>
concept HasEnumToString = requires(T value) {
	{ EnumToString(value) } -> std::same_as<FString>;
};

namespace VulRuntime::Field
{
	/**
	 * A single entry in a FPath, either a string (for objects) or numeric (for arrays) index.
	 */
	using FPathItem = TVariant<FString, int>;
	
	/**
	 * How we represent serialization paths, akin to JSON path.
	 *
	 * This keeps track of where we are in deserialization/serialization operations.
	 */
	using FPath = TArray<FPathItem>;
	
	/**
	 * Returns true if we consider the given value empty.
	 *
	 * Empty if any of the following are true.
	 *   - is an invalid JSON value
	 *   - is null
	 *   - is an empty string
	 *   - is an array of length 0, or all elements in the array empty (checked recursively).
	 *   - is an empty object, or all values in the object are empty (checked recursively).
	 */
	VULRUNTIME_API bool IsEmpty(const TSharedPtr<FJsonValue>& Value);

	/**
	 * Converts a Path to its string form, e.g. ".foo.bar.arr[2].baz".
	 */
	VULRUNTIME_API FString PathStr(const FPath& Path);

	/**
	 * Returns true if Match satisfies path. This supports wildcard indices in place of numerics.
	 *
	 * E.g. Path=".foo.arr[1]" matches Match=".foo.arr[*]".
	 *
	 * This requires a full match along the whole path and does not support sub-tree matching.
	 * Wildcards are supported for non-numeric properties, but will only match a single property.
	 * E.g. ".foo.*" will match ".foo.bar", but not ".foo.bar.baz".
	 *
	 * This match ignores case.
	 */
	VULRUNTIME_API bool PathMatch(const FPath& Path, const FString& Match);

	/**
	 * Helper to return the string representation of the given JSON type.
	 */
	VULRUNTIME_API FString JsonTypeToString(const EJson Type);

	template <typename CharType = TCHAR, typename PrintPolicy = TCondensedJsonPrintPolicy<CharType>>
	FString JsonToString(const TSharedPtr<FJsonValue>& Json)
	{
		// UE Json serialization doesn't support non-objects.
		if (Json->Type == EJson::String)
		{
			return "\"" + Json->AsString() + "\"";
		}

		// Handles number, bool (no quotes needed).
		FString AsStr; 
		if (Json->TryGetString(AsStr))
		{
			return AsStr;
		}

		FString Out;
		auto Writer = TJsonWriterFactory<CharType, PrintPolicy>::Create(&Out);
		if (TArray<TSharedPtr<FJsonValue>>* AsArray; Json->TryGetArray(AsArray))
		{
			if (!FJsonSerializer::Serialize(*AsArray, Writer))
			{
				return "";
			}

			return Out;
		}
		
		if (!FJsonSerializer::Serialize(Json, "", Writer))
		{
			return "";
		}

		return Out;
	}

	/**
	 * Helper returning a string indicating the type T.
	 *
	 * Based on compiler macros, avoiding typeid(); the output will not be clean.
	 */
	template <typename T>
	FString TypeInfo()
	{
#if defined(__clang__) || defined(__GNUC__)
		return ANSI_TO_TCHAR(__PRETTY_FUNCTION__);
#elif defined(_MSC_VER)
		return ANSI_TO_TCHAR(__FUNCSIG__);
#else
		checkf(false, "Compiler unable to infer typeinfo")
#endif
	}
 
	/**
	 * Returns a unique identifier string for the C++ type `T`.
	 * 
	 * This identifier is consistent within a single build and runtime,
	 * and is used by the Vul reflection system for type registration and lookup.
	 * It can also be used in diagnostics or error messages to reference types clearly.
	 *
	 * Note: This ID is not stable across builds and should not be used for persistent storage
	 * or communication between different binaries.
	 */
	template <typename T>
	FString TypeId()
	{
		return FMD5::HashAnsiString(*TypeInfo<T>());
	}
}