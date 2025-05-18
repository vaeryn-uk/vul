#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

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
		FString Out;
		auto Writer = TJsonWriterFactory<CharType, PrintPolicy>::Create(&Out);
		if (!FJsonSerializer::Serialize(Json, "", Writer))
		{
			return "";
		}

		return Out;
	}
}