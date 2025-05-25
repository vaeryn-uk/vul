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
	 * When defining resolvers for your types, this must return true.
	 *
	 * TODO: For deserialization, note your type must support copy assignment for now.
	 *       I.e. std::is_copy_assignable_v<T>, which precludes TUniquePtr. Fix this
	 *       in the future to support customized copying.
	 */
	static bool SupportsRef() { return false; }
	
	/**
	 * Resolves a ref for the given value. This must set a string-like JSON value,
	 * such as an FJsonValueString or FJsonValueNumber, and return true to indicate
	 * support.
	 *
	 * This default implementation returns false; i.e. no support for shared references.
	 */
	static bool Resolve(
		const T& Value,
		TSharedPtr<FJsonValue>& Out,
		struct FVulFieldSerializationState& SerializationState
	) {
		return false;
	}
};

template <typename T>
struct TVulFieldRefResolver<TSharedPtr<T>>
{
	static bool SupportsRef() { return TVulFieldRefResolver<T>::SupportsRef(); }
	
	static bool Resolve(
		const TSharedPtr<T>& Value,
		TSharedPtr<FJsonValue>& Out,
		FVulFieldSerializationState& SerializationState
	) {
		if (!Value.IsValid())
		{
			return false;
		}
		
		return TVulFieldRefResolver<T>::Resolve(*Value.Get(), Out, SerializationState);
	}
};

template <typename T>
struct TVulFieldRefResolver<T*>
{
	static bool SupportsRef() { return TVulFieldRefResolver<T>::SupportsRef(); }
	
	static bool Resolve(
		const T* const& Value,
		TSharedPtr<FJsonValue>& Out,
		FVulFieldSerializationState& SerializationState
	) {
		if (Value == nullptr)
		{
			return false;
		}
		
		return TVulFieldRefResolver<T>::Resolve(*Value, Out, SerializationState);
	}
};

template <typename T>
struct TVulFieldRefResolver<TWeakObjectPtr<T>>
{
	static bool SupportsRef() { return TVulFieldRefResolver<T>::SupportsRef(); }
	
	static bool Resolve(
		const TWeakObjectPtr<T>& Value,
		TSharedPtr<FJsonValue>& Out,
		FVulFieldSerializationState& SerializationState
	) {
		if (!Value.IsValid())
		{
			return false;
		}
		
		return TVulFieldRefResolver<T>::Resolve(*Value.Get(), Out, SerializationState);
	}
};