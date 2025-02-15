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
		Out.Read = [](void* Ptr, TSharedPtr<FJsonValue>& Out)
		{
			Out = FVulFieldSerializer<T>::Serialize(*reinterpret_cast<T*>(Ptr));

			return true;
		};
		Out.Write = [](const TSharedPtr<FJsonValue>& Value, void* Ptr)
		{
			return FVulFieldSerializer<T>::Deserialize(Value, *reinterpret_cast<T*>(Ptr));
		};
		return Out;
	}
	template <typename V>
	static FVulField Create(TArray<V>* Ptr)
	{
		FVulField Out;
		Out.Ptr = Ptr;
		Out.Read = [](void* Ptr, TSharedPtr<FJsonValue>& Out)
		{
			TArray<TSharedPtr<FJsonValue>> Array;

			for (const auto& Entry : *reinterpret_cast<TArray<V>*>(Ptr))
			{
				Array.Add(FVulFieldSerializer<V>::Serialize(Entry));
			}

			Out = MakeShared<FJsonValueArray>(Array);

			return true;
		};
		Out.Write = [](const TSharedPtr<FJsonValue>& Value, void* Ptr)
		{
			TArray<V>* Array = reinterpret_cast<TArray<V>*>(Ptr);
			Array->Reset();

			TArray<TSharedPtr<FJsonValue>>* JsonArray;
			if (Value->Type != EJson::Array || !Value->TryGetArray(JsonArray))
			{
				return false;
			}

			for (const auto Entry : *JsonArray)
			{
				V ValueObj;
				if (!FVulFieldSerializer<V>::Deserialize(Entry, ValueObj))
				{
					return false;
				}

				Array->Add(ValueObj);
			}

			return true;
		};
		
		return Out;
	}
	
	template <typename K, typename V>
	static FVulField Create(TMap<K, V>* Ptr)
	{
		FVulField Out;
		Out.Ptr = Ptr;
		Out.Read = [](void* Ptr, TSharedPtr<FJsonValue>& Out)
		{
			auto Obj = MakeShared<FJsonObject>();

			for (const auto& Entry : *reinterpret_cast<TMap<K, V>*>(Ptr))
			{
				const auto KeyJson = FVulFieldSerializer<K>::Serialize(Entry.Key);
				
				FString KeyStr;
				if (KeyJson->Type != EJson::String || !KeyJson->TryGetString(KeyStr))
				{
					return false;
				}

				Obj->Values.Add(KeyStr, FVulFieldSerializer<V>::Serialize(Entry.Value));
			}

			Out = MakeShared<FJsonValueObject>(Obj);

			return true;
		};
		Out.Write = [](const TSharedPtr<FJsonValue>& Value, void* Ptr)
		{
			TMap<K, V>* Map = reinterpret_cast<TMap<K, V>*>(Ptr);
			Map->Reset();

			TSharedPtr<FJsonObject>* Obj;
			if (Value->Type != EJson::Object || !Value->TryGetObject(Obj))
			{
				return false;
			}

			for (const auto Entry : (*Obj)->Values)
			{
				K Key;
				if (!FVulFieldSerializer<K>::Deserialize(MakeShared<FJsonValueString>(Entry.Key), Key))
				{
					return false;
				}

				V ValueObj;
				if (!FVulFieldSerializer<V>::Deserialize(Entry.Value, ValueObj))
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
	template <typename V>
	static FVulField Create(const TArray<V>* Ptr) { return Create<V>(const_cast<TArray<V>*>(Ptr)); }
	template <typename K, typename V>
	static FVulField Create(const TMap<K, V>* Ptr) { return Create<K, V>(const_cast<TMap<K, V>*>(Ptr)); }

	bool Set(const TSharedPtr<FJsonValue>& Value) { return Write(Value, Ptr); }
	bool Get(TSharedPtr<FJsonValue>& Out) const { return Read(Ptr, Out); }

	template <typename CharType = TCHAR>
	bool SetFromJsonString(const FString& JsonStr)
	{
		TSharedPtr<FJsonValue> ParsedJson;
		auto Reader = TJsonReaderFactory<CharType>::Create(JsonStr);
		if (!FJsonSerializer::Deserialize(Reader, ParsedJson) || !ParsedJson.IsValid())
		{
			return false;
		}

		return Set(ParsedJson);
	}

	template <typename CharType = TCHAR, typename PrintPolicy = TCondensedJsonPrintPolicy<CharType>>
	bool ToJsonString(FString& Out) const
	{
		TSharedPtr<FJsonValue> Obj;
		if (!Get(Obj) || !Obj.IsValid())
		{
			return false;
		}
		
		auto Writer = TJsonWriterFactory<CharType, PrintPolicy>::Create(&Out);
		if (!FJsonSerializer::Serialize(Obj, "", Writer))
		{
			return false;
		}

		return true;
	}

private:
	void* Ptr = nullptr;
	TFunction<bool (void*, TSharedPtr<FJsonValue>&)> Read;
	TFunction<bool (const TSharedPtr<FJsonValue>&, void*)> Write;
};