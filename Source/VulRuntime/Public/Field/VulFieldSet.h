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
	void Add(const FVulField& Field, const FString& Identifier, bool ReadOnly = false);

	bool Serialize(TSharedPtr<FJsonValue>& Out) const;
	bool Deserialize(const TSharedPtr<FJsonValue>& Data);
	
	template <typename CharType = TCHAR, typename PrintPolicy = TCondensedJsonPrintPolicy<CharType>>
	bool SerializeToJson(FString& Out) const
	{
		TSharedPtr<FJsonValue> Json;
		if (!Serialize(Json))
		{
			return false;
		}
		
		auto Writer = TJsonWriterFactory<CharType, PrintPolicy>::Create(&Out);
		return FJsonSerializer::Serialize(Json, "", Writer);
	}

	template <typename CharType = TCHAR>
	bool DeserializeFromJson(const FString& JsonStr)
	{
		TSharedPtr<FJsonValue> ParsedJson;
		auto Reader = TJsonReaderFactory<CharType>::Create(JsonStr);
		if (!FJsonSerializer::Deserialize(Reader, ParsedJson) || !ParsedJson.IsValid())
		{
			return false;
		}
		
		return Deserialize(ParsedJson);
	}
private:
	struct FFieldDescription
	{
		FVulField Field;
		bool ReadOnly = false;
		FString Identifier;
	};

	TMap<FString, FFieldDescription> Fields;
};
