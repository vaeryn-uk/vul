#pragma once

/**
 * This struct must be implemented for each type you want to support with FVulField de/serialization.
 *
 * See VulFieldCommonSerializers.h for example implementations.
 */
template <typename T>
struct TVulFieldSerializer
{
	/**
	 * A hook that will be called prior to any serialization or deserialization.
	 *
	 * Can be used to set up custom flags.
	 */
	static void Setup()
	{
		
	}
	
	/**
	 * Given the Value, turn it in to its serialized form, setting it to Out.
	 *
	 * Return true if the Serialization succeeds.
	 *
	 * Ctx can be used to record errors and chain serialization for nested data.
	 */
	static bool Serialize(const T& Value, TSharedPtr<FJsonValue>& Out, struct FVulFieldSerializationContext& Ctx)
	{
#if defined(_MSC_VER)
		static_assert(sizeof(T) == -1, "Error: " __FUNCSIG__ " is not defined.");
#else
		static_assert(sizeof(T) == -1, "Error: function is not defined.");
#endif
		return false;
	}

	/**
	 * Given some serialized Data, turn it back in its correctly typed form, setting it to Out.
	 *
	 * Return true if the Deserialization succeeds.
	 *
	 * Ctx can be used to record errors and chain deserialization for nested data.
	 */
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out, struct FVulFieldDeserializationContext& Ctx)
	{
#if defined(_MSC_VER)
		static_assert(sizeof(T) == -1, "Error: " __FUNCSIG__ " is not defined.");
#else
		static_assert(sizeof(T) == -1, "Error: function is not defined.");
#endif
		return false;
	}
};