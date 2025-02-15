#pragma once

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
			// TODO.
			return false;
		};
		return Out;
	}

	bool Set(const TSharedPtr<FJsonValue>& Value) { return Write(Value, Ptr); }
	bool Get(TSharedPtr<FJsonValue>& Out) const { return Read(Ptr, Out); }

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