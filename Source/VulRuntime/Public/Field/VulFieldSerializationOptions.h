#pragma once

#include "CoreMinimal.h"
#include "VulFieldUtil.h"
#include "VulFieldRefResolver.h"
#include "VulRuntimeSettings.h"
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

/**
 * If set, serialization of objects will be annotated with a "VulType" property within which
 * tags that object as being of that VULFLD-registered type.
 */
const static FString VulFieldSerializationFlag_AnnotateTypes = "vul.annotate-types";

struct VULRUNTIME_API FVulFieldSerializationFlags
{
	/**
	 * Sets a new value, returning the effective old value.
	 * 
	 * Optionally set Path to only apply at that point in the de/serialization tree.
	 *
	 * The path expects dot-separated with numeric and property wildcards, e.g. ".foo.*.arr[*].baz".
	 */
	void Set(const FString& Option, const bool Value = true, const FString& Path = "");

	bool IsEnabled(const FString& Option, const VulRuntime::Field::FPath& Path) const;

	template <typename T>
	bool SupportsReferencing(const VulRuntime::Field::FPath& Path) const
	{
		const bool TypeSupportsRef = TVulFieldRefResolver<T>::SupportsRef();
		return TypeSupportsRef && IsEnabled(VulFieldSerializationFlag_Referencing, Path);
	}

	static void RegisterDefault(const FString& Option, const bool Default)
	{
		GlobalDefaults.Add(Option, Default);
	}
	
private:
	TMap<FString, TMap<FString, bool>> PathFlags;
	
	bool Resolve(const FString& Option, const VulRuntime::Field::FPath& Path) const;

	static TMap<FString, bool> GlobalDefaults;
};