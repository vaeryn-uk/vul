#pragma once

#include "VulFieldSerializationContext.h"

template<>
struct FVulFieldSerializer<bool>
{
	static bool Serialize(const bool& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueBoolean>(Value);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, bool& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.Errors.RequireJsonType(Data, EJson::Boolean))
		{
			return false;
		}

		return Ctx.Errors.AddIfNot(Data->TryGetBool(Out), TEXT("serialized data is not a bool"));
	}
};

template<>
struct FVulFieldSerializer<int>
{
	static bool Serialize(const int& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueNumber>(Value);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, int& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.Errors.RequireJsonType(Data, EJson::Number))
		{
			return false;
		}

		return Ctx.Errors.AddIfNot(Data->TryGetNumber(Out), TEXT("serialized data is not a number"));
	}
};

template<>
struct FVulFieldSerializer<FString>
{
	static bool Serialize(const FString& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueString>(Value);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, FString& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.Errors.RequireJsonType(Data, EJson::String))
		{
			return false;
		}

		return Ctx.Errors.AddIfNot(Data->TryGetString(Out), TEXT("serialized data is not a string"));
	}
};

template<typename V>
struct FVulFieldSerializer<TArray<V>>
{
	static bool Serialize(const TArray<V>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		TArray<TSharedPtr<FJsonValue>> ArrayItems;
		for (const auto Item : Value)
		{
			TSharedPtr<FJsonValue> ToAdd;
			if (!Ctx.Serialize<V>(Item, ToAdd))
			{
				return false;
			}

			ArrayItems.Add(ToAdd);
		}

		Out = MakeShared<FJsonValueArray>(ArrayItems);
		
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TArray<V>& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.Errors.RequireJsonType(Data, EJson::Array))
		{
			return false;
		}
		
		Out.Reset();

		for (const auto Entry : Data->AsArray())
		{
			V Value;
			if (!Ctx.Deserialize<V>(Entry, Value))
			{
				return false;
			}

			Out.Add(Value);
		}
		
		return true;
	}
};

template <typename K, typename V>
struct FVulFieldSerializer<TMap<K, V>>
{
	static bool Serialize(const TMap<K, V>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		auto OutObj = MakeShared<FJsonObject>();
		
		for (const auto Entry : Value)
		{
			TSharedPtr<FJsonValue> ItemKey;
			if (!Ctx.Serialize<K>(Entry.Key, ItemKey))
			{
				return false;
			}

			if (!Ctx.Errors.RequireJsonType(ItemKey, EJson::String))
			{
				return false;
			}
			
			TSharedPtr<FJsonValue> ItemValue;
			if (!Ctx.Serialize<V>(Entry.Value, ItemValue))
			{
				return false;
			}

			OutObj->Values.Add(ItemKey->AsString(), ItemValue);
		}

		Out = MakeShared<FJsonValueObject>(OutObj);

		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TMap<K, V>& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.Errors.RequireJsonType(Data, EJson::Object))
		{
			return false;
		}

		Out.Reset();

		for (const auto Entry : Data->AsObject()->Values)
		{
			K KeyToAdd;
			if (!Ctx.Deserialize<K>(MakeShared<FJsonValueString>(Entry.Key), KeyToAdd))
			{
				return false;
			}
			
			V ValueToAdd;
			if (!Ctx.Deserialize<V>(Entry.Value, ValueToAdd))
			{
				return false;
			}

			Out.Add(KeyToAdd, ValueToAdd);
		}
		
		return true;
	}
};

template <typename T>
struct FVulFieldSerializer<TOptional<T>>
{
	static bool Serialize(const TOptional<T>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (!Value.IsSet())
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}

		return Ctx.Serialize<T>(Value.GetValue(), Out);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TOptional<T>& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (Data->Type == EJson::Null)
		{
			Out = TOptional<T>{};
			return true;
		}

		T Inner;
		if (!Ctx.Deserialize<T>(Data, Inner))
		{
			return false;
		}

		Out = Inner;
		return true;
	}
};

template <typename T>
struct FVulFieldSerializer<TSharedPtr<T>>
{
	static bool Serialize(const TSharedPtr<T>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (!Value.IsValid())
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}
		
		return Ctx.Serialize<T>(*Value.Get(), Out);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TSharedPtr<T>& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (Data->Type == EJson::Null)
		{
			Out = nullptr;
			return true;
		}

		Out = MakeShared<T>();
		if (!Ctx.Deserialize<T>(Data, *Out.Get()))
		{
			return false;
		}

		return true;
	}
};

template <typename T>
struct FVulFieldSerializer<TSharedRef<T>>
{
	static bool Serialize(const TSharedRef<T>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		return Ctx.Serialize<T>(Value.Get(), Out);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TSharedRef<T>& Out, FVulFieldDeserializationContext& Ctx)
	{
		TSharedPtr<T> Inner = MakeShared<T>();
		if (!Ctx.Deserialize<T>(Data, *Inner.Get()))
		{
			return false;
		}

		Out = Inner.ToSharedRef();

		return true;
	}
};