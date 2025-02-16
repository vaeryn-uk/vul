#pragma once

#include "CoreMinimal.h"
#include "VulFieldSerializers.h"
#include "UObject/Object.h"

/**
 * A field that can be conveniently serialized/deserialized.
 *
 * This is a wrapper around a pointer that allows Get and Set operations.
 * For the de/serialization itself, FVulFieldSerializer must be implemented
 * for the type you're wrapping.
 *
 * Note the FVulField and associated APIs deal with FJsonValue. This is the chosen
 * portable intermediate representation, though the fields API has been
 * designed to be a more generic de/serialization toolkit than just strictly JSON.
 *
 * When describing your types' fields, you'll likely want a VulFieldSet.
 */
struct FVulField
{
	template <typename T>
	static FVulField Create(T* Ptr)
	{
		FVulField Out;
		Out.Ptr = Ptr;
		Out.Read = [](void* Ptr, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
		{
			return FVulFieldSerializer<T>::Serialize(*reinterpret_cast<T*>(Ptr), Out, Ctx);
		};
		Out.Write = [](const TSharedPtr<FJsonValue>& Value, void* Ptr, FVulFieldDeserializationContext& Ctx)
		{
			return FVulFieldSerializer<T>::Deserialize(Value, *reinterpret_cast<T*>(Ptr), Ctx);
		};
		return Out;
	}
	
	template <typename K, typename V>
	static FVulField Create(TMap<K, V>* Ptr)
	{
		FVulField Out;
		Out.Ptr = Ptr;
		Out.Read = [](void* Ptr, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
		{
			auto Obj = MakeShared<FJsonObject>();

			for (const auto& Entry : *reinterpret_cast<TMap<K, V>*>(Ptr))
			{
				TSharedPtr<FJsonValue> KeyJson;
				if (!FVulFieldSerializer<K>::Serialize(Entry.Key, KeyJson, Ctx))
				{
					return false;
				}

				if (!Ctx.Errors.RequireJsonType(KeyJson, EJson::String))
				{
					return false;
				}

				TSharedPtr<FJsonValue> ValueJson;
				if (!FVulFieldSerializer<V>::Serialize(Entry.Value, ValueJson, Ctx))
				{
					return false;
				}
				
				Obj->Values.Add(KeyJson->AsString(), ValueJson);
			}

			Out = MakeShared<FJsonValueObject>(Obj);

			return true;
		};
		Out.Write = [](const TSharedPtr<FJsonValue>& Value, void* Ptr, FVulFieldDeserializationContext& Ctx)
		{
			TMap<K, V>* Map = reinterpret_cast<TMap<K, V>*>(Ptr);
			Map->Reset();

			if (!Ctx.Errors.RequireJsonType(Value, EJson::Object))
			{
				return false;
			}

			for (const auto Entry : Value->AsObject()->Values)
			{
				K Key;
				if (!FVulFieldSerializer<K>::Deserialize(MakeShared<FJsonValueString>(Entry.Key), Key, Ctx))
				{
					return false;
				}

				V ValueObj;
				if (!FVulFieldSerializer<V>::Deserialize(Entry.Value, ValueObj, Ctx))
				{
					return false;
				}

				Map->Add(Key, ValueObj);
			}

			return true;
		};
		
		return Out;
	}

	/**
	 * Overloads that allow for declaration of FVulFields in const contexts.
	 * 
	 * Warning: the constness of the property will be lost (as a FVulField is writable
	 * by design). This must never be used on properties within objects that are defined
	 * as const; this is only intended for use within mutable objects (e.g. constructed via
	 * MakeShared) when defining fieldsets for de/serialization.
	 */
	template <typename T>
	static FVulField Create(const T* Ptr) { return Create<T>(const_cast<T*>(Ptr)); }
	template <typename K, typename V>
	static FVulField Create(const TMap<K, V>* Ptr) { return Create<K, V>(const_cast<TMap<K, V>*>(Ptr)); }

	bool Deserialize(const TSharedPtr<FJsonValue>& Value);
	bool Deserialize(const TSharedPtr<FJsonValue>& Value, FVulFieldDeserializationContext& Ctx);
	bool Serialize(TSharedPtr<FJsonValue>& Out) const;
	bool Serialize(TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx) const;

	template <typename CharType = TCHAR>
	bool DeserializeFromJson(const FString& JsonStr, FVulFieldDeserializationContext& Ctx)
	{
		TSharedPtr<FJsonValue> ParsedJson;
		auto Reader = TJsonReaderFactory<CharType>::Create(JsonStr);
		if (!FJsonSerializer::Deserialize(Reader, ParsedJson) || !ParsedJson.IsValid())
		{
			Ctx.Errors.Add(TEXT("cannot parse invalid JSON string"));
			return false;
		}

		return Deserialize(ParsedJson, Ctx);
	}
	
	template <typename CharType = TCHAR>
	bool DeserializeFromJson(const FString& JsonStr)
	{
		FVulFieldDeserializationContext Ctx;
		return DeserializeFromJson<CharType>(JsonStr, Ctx);
	}

	template <typename CharType = TCHAR, typename PrintPolicy = TCondensedJsonPrintPolicy<CharType>>
	bool SerializeToJson(FString& Out, FVulFieldSerializationContext& Ctx) const
	{
		TSharedPtr<FJsonValue> Obj;
		if (!Serialize(Obj, Ctx) || !Obj.IsValid())
		{
			return false;
		}
		
		auto Writer = TJsonWriterFactory<CharType, PrintPolicy>::Create(&Out);
		if (!FJsonSerializer::Serialize(Obj, "", Writer))
		{
			Ctx.Errors.Add(TEXT("serialization of JSON string failed"));
			return false;
		}

		return true;
	}
	
	template <typename CharType = TCHAR, typename PrintPolicy = TCondensedJsonPrintPolicy<CharType>>
	bool SerializeToJson(FString& Out) const
	{
		FVulFieldSerializationContext Ctx;
		return SerializeToJson<CharType, PrintPolicy>(Out, Ctx);
	}

private:
	void* Ptr = nullptr;
	TFunction<bool (void*, TSharedPtr<FJsonValue>&, FVulFieldSerializationContext& Ctx)> Read;
	TFunction<bool (const TSharedPtr<FJsonValue>&, void*, FVulFieldDeserializationContext& Ctx)> Write;
};