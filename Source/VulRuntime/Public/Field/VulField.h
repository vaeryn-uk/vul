﻿#pragma once

#include "CoreMinimal.h"
#include "VulFieldSerializers.h"
#include "UObject/Object.h"

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

		FString OutputString;
		auto Writer = TJsonWriterFactory<CharType, PrintPolicy>::Create(&OutputString);

		if (!FJsonSerializer::Serialize(Obj, "", Writer))
		{
			return false;
		}

		// Set the output string
		Out = OutputString;
		return true;
	}

private:
	void* Ptr = nullptr;
	TFunction<bool (void*, TSharedPtr<FJsonValue>&)> Read;
	TFunction<bool (const TSharedPtr<FJsonValue>&, void*)> Write;
};