#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * During serialization, referencing will cause supporting types to be replaced by an identifier
 * when repeated instances of that type appear; only the first will be the full serialized
 * definition.
 *
 * During deserialization, referencing will look for and interpret identifiers and match
 * it back to a previous full definition. For pointer types specifically, this will
 *
 * Types will need a TVulFieldRefResolver to convert an object to its identifier.
 *
 * Default: on.
 */
const static FString VulFieldSerializationFlag_Referencing = "vul.referencing";

/**
 * When serializing UObjects that are assets, this flag will represent them as a string
 * (asset path), so that it can be loaded to the exact same asset when deserializing.
 *
 * Default: on.
 */
const static FString VulFieldSerializationFlag_AssetReferencing = "vul.asset-referencing";

struct VULRUNTIME_API FVulFieldSerializationFlags
{
	void Set(const FString& Option, const bool Value = true)
	{
		Flags.Add(Option, Value);
	}

	bool IsEnabled(const FString& Option) const;

	static void RegisterDefault(const FString& Option, const bool Default)
	{
		GlobalDefaults.Add(Option, Default);
	}
	
private:
	TMap<FString, int> Flags;

	bool Resolve(const FString& Option) const;

	static TMap<FString, bool> GlobalDefaults;
};