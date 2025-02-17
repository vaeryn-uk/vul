#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Field/VulField.h"

/**
 * A collection of FVulFields that can be de/serialized.
 *
 * This is designed to allow your types to expose a field set that describes it,
 * then we can easily de/serialize instances of that type at all at once.
 *
 * This is akin to an Object in JSON.
 */
struct FVulFieldSet
{
	/**
	 * Adds a field to the set. If read only, the field will only be
	 * serialized, but ignored for deserialization. 
	 */
	void Add(const FVulField& Field, const FString& Identifier);

	/**
	 * Adds a virtual field - one whose value is derived from a function
	 * call. These are only relevant when serializing a field set.
	 *
	 * This is useful for adding additional data to your serialized outputs,
	 * where the data is supplemental and not required to reconstitute an
	 * object correctly.
	 */
	template <typename T>
	void Add(TFunction<T ()> Fn, const FString& Identifier)
	{
		Fns.Add(Identifier, [Fn](TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
		{
			return Ctx.Serialize<T>(Fn(), Out);
		});
	}

	bool Serialize(TSharedPtr<FJsonValue>& Out) const;
	bool Serialize(TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx) const;
	bool Deserialize(const TSharedPtr<FJsonValue>& Data);
	bool Deserialize(const TSharedPtr<FJsonValue>& Data, FVulFieldDeserializationContext& Ctx);
	
	template <typename CharType = TCHAR, typename PrintPolicy = TCondensedJsonPrintPolicy<CharType>>
	bool SerializeToJson(FString& Out, FVulFieldSerializationContext& Ctx) const
	{
		TSharedPtr<FJsonValue> Json;
		if (!Serialize(Json))
		{
			return false;
		}
		
		auto Writer = TJsonWriterFactory<CharType, PrintPolicy>::Create(&Out);
		return FJsonSerializer::Serialize(Json, "", Writer);
	}
	
	template <typename CharType = TCHAR, typename PrintPolicy = TCondensedJsonPrintPolicy<CharType>>
	bool SerializeToJson(FString& Out) const
	{
		FVulFieldSerializationContext Ctx;
		return SerializeToJson<CharType, PrintPolicy>(Out, Ctx);
	}

	template <typename CharType = TCHAR>
	bool DeserializeFromJson(const FString& JsonStr, FVulFieldDeserializationContext& Ctx)
	{
		TSharedPtr<FJsonValue> ParsedJson;
		auto Reader = TJsonReaderFactory<CharType>::Create(JsonStr);
		if (!FJsonSerializer::Deserialize(Reader, ParsedJson) || !ParsedJson.IsValid())
		{
			return false;
		}
		
		return Deserialize(ParsedJson);
	}
	
	template <typename CharType = TCHAR>
	bool DeserializeFromJson(const FString& JsonStr)
	{
		FVulFieldDeserializationContext Ctx;
		return DeserializeFromJson<CharType>(JsonStr, Ctx);
	}
private:
	TMap<FString, FVulField> Fields;
	TMap<FString, TFunction<bool (TSharedPtr<FJsonValue>&, FVulFieldSerializationContext&)>> Fns;
};
