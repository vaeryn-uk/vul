#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * Responsible for resolving a string-like reference for a value in FVulField
 * serialization and deserialization.
 *
 * Refs yield a condensed representation of data if it's been seen before, where
 * subsequent appearances are some form of ID. During deserialization, a ref will
 * be resolved back to the fully-defined, previously seen instance.
 *
 * Implement this for your type to add shared reference support to your de/serialization
 * process.
 */
template <typename T>
struct TVulFieldRefResolver
{
	/**
	 * This indicates to the FVulField system whether to use the referencing system at all.
	 *
	 * When defining resolvers for your types, this must be set as true.
	 */
	static constexpr bool SupportsRef = false;
	
	/**
	 * Resolves a ref for the given value. This must set a string-like JSON value,
	 * such as an FJsonValueString or FJsonValueNumber, and return true to indicate
	 * support.
	 *
	 * This default implementation returns false; i.e. no support for shared references.
	 */
	static bool Resolve(const T& Value, TSharedPtr<FJsonValue>& Out) { return false; }
};

template <typename T>
struct TVulFieldRefResolver<TSharedPtr<T>>
{
	static constexpr bool SupportsRef = true;
	
	static bool Resolve(const TSharedPtr<T>& Value, TSharedPtr<FJsonValue>& Out)
	{
		if (!Value.IsValid())
		{
			return false;
		}
		
		return TVulFieldRefResolver<T>::Resolve(*Value.Get(), Out);
	}
};

template <typename T>
struct TVulFieldRefResolver<T*>
{
	static constexpr bool SupportsRef = true;
	
	static bool Resolve(const T* const& Value, TSharedPtr<FJsonValue>& Out)
	{
		if (Value == nullptr)
		{
			return false;
		}
		
		return TVulFieldRefResolver<T>::Resolve(*Value, Out);
	}
};