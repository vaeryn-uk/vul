#pragma once

#include "VulFieldSerializationContext.h"
#include "Misc/VulEnum.h"

template<>
struct TVulFieldSerializer<bool>
{
	static bool Serialize(const bool& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueBoolean>(Value);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, bool& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::Boolean))
		{
			return false;
		}

		return Ctx.State.Errors.AddIfNot(Data->TryGetBool(Out), TEXT("serialized data is not a bool"));
	}
};

template <typename T>
concept IsNumeric = std::is_arithmetic_v<T>;

template<IsNumeric T>
struct TVulFieldSerializer<T>
{
	static bool Serialize(const T& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		constexpr bool IsFP = std::is_floating_point_v<T>;
		if (IsFP)
		{
			Out = MakeShared<FJsonValueNumberString>(FString::Printf(TEXT("%.*f"), Ctx.DefaultPrecision, Value));
		} else
		{
			Out = MakeShared<FJsonValueNumber>(Value);
		}
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::Number))
		{
			return false;
		}

		return Ctx.State.Errors.AddIfNot(Data->TryGetNumber(Out), TEXT("serialized data is not a number"));
	}
};

template<>
struct TVulFieldSerializer<FString>
{
	static bool Serialize(const FString& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueString>(Value);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, FString& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::String))
		{
			return false;
		}

		return Ctx.State.Errors.AddIfNot(Data->TryGetString(Out), TEXT("serialized data is not a string"));
	}
};

template<>
struct TVulFieldSerializer<FName>
{
	static bool Serialize(const FName& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueString>(Value.ToString());
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, FName& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::String))
		{
			return false;
		}

		Out = FName(Data->AsString());
		return true;
	}
};

template<typename V>
struct TVulFieldSerializer<TArray<V>>
{
	static bool Serialize(const TArray<V>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		TArray<TSharedPtr<FJsonValue>> ArrayItems;
		
		int I = 0;
		for (const auto Item : Value)
		{
			TSharedPtr<FJsonValue> ToAdd;
			if (!Ctx.Serialize<V>(Item, ToAdd, VulRuntime::Field::FPathItem(TInPlaceType<int>(), I++)))
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
		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::Array))
		{
			return false;
		}
		
		Out.Reset();

		int I = 0;
		for (const auto Entry : Data->AsArray())
		{
			V Value;
			if (!Ctx.Deserialize<V>(Entry, Value, VulRuntime::Field::FPathItem(TInPlaceType<int>(), I++)))
			{
				return false;
			}

			Out.Add(Value);
		}
		
		return true;
	}
};

template <typename K, typename V>
struct TVulFieldSerializer<TMap<K, V>>
{
	static bool Serialize(const TMap<K, V>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		auto OutObj = MakeShared<FJsonObject>();
		
		for (const auto Entry : Value)
		{
			TSharedPtr<FJsonValue> ItemKey;
			if (!Ctx.Serialize<K>(Entry.Key, ItemKey, VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "__key__")))
			{
				return false;
			}

			if (!Ctx.State.Errors.RequireJsonType(ItemKey, EJson::String))
			{
				return false;
			}
			
			TSharedPtr<FJsonValue> ItemValue;
			if (!Ctx.Serialize<V>(Entry.Value, ItemValue, VulRuntime::Field::FPathItem(TInPlaceType<FString>(), ItemKey->AsString())))
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
		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::Object))
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
struct TVulFieldSerializer<TOptional<T>>
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
struct TVulFieldSerializer<TSharedPtr<T>>
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
struct TVulFieldSerializer<TUniquePtr<T>>
{
	static bool Serialize(const TUniquePtr<T>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (!Value.IsValid())
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}
		
		return Ctx.Serialize<T>(*Value.Get(), Out);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TUniquePtr<T>& Out, FVulFieldDeserializationContext& Ctx)
	{
		static_assert(std::is_move_constructible<T>::value, "T must be move-constructible for TUniquePtr Vul field deserialization");
		
		if (Data->Type == EJson::Null)
		{
			Out = nullptr;
			return true;
		}

		Out = MakeUnique<T>();
		if (!Ctx.Deserialize<T>(Data, *Out.Get()))
		{
			return false;
		}

		return true;
	}
};

template <typename T>
struct TVulFieldSerializer<TWeakObjectPtr<T>>
{
	static bool Serialize(const TWeakObjectPtr<T>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (!Value.IsValid())
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}
		
		return Ctx.Serialize<T>(*Value.Get(), Out);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TWeakObjectPtr<T>& Out, FVulFieldDeserializationContext& Ctx)
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
struct TVulFieldSerializer<T*>
{
	static bool Serialize(const T* const& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (Value == nullptr)
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}
		
		return Ctx.Serialize<T>(*Value, Out);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T*& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (Data->Type == EJson::Null)
		{
			Out = nullptr;
			return true;
		}

		Out = new T;
		if (!Ctx.Deserialize<T>(Data, *Out))
		{
			return false;
		}

		return true;
	}
};

template <typename T>
struct TVulFieldSerializer<TSharedRef<T>>
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

template <typename T>
concept HasEnumToString = requires(T value) {
	{ EnumToString(value) } -> std::same_as<FString>;
};

template <HasEnumToString T>
struct TVulFieldSerializer<T>
{
	static bool Serialize(const T& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		Out = MakeShared<FJsonValueString>(EnumToString(Value));
		return true;
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::String))
		{
			return false;
		}

		if (!VulRuntime::Enum::FromString(Data->AsString(), Out))
		{
			Ctx.State.Errors.Add(TEXT("cannot interpret enum value \"%s\""), *Data->AsString());
			return false;
		}
		
		return true;
	}
};


template<typename T, typename S>
struct TVulFieldSerializer<TPair<T,S>>
{
	static bool Serialize(const TPair<T,S>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		TArray<TSharedPtr<FJsonValue>> Entries;

		Entries.AddDefaulted(2);

		if (!Ctx.Serialize(Value.Key, Entries[0], VulRuntime::Field::FPathItem(TInPlaceType<int>(), 0)))
		{
			return false;
		}

		TSharedPtr<FJsonValue> SerializedValue;
		if (!Ctx.Serialize(Value.Value, Entries[1], VulRuntime::Field::FPathItem(TInPlaceType<int>(), 1)))
		{
			return false;
		}

		Out = MakeShared<FJsonValueArray>(Entries);
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TPair<T,S>& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::Array))
		{
			return false;
		}

		if (Data->AsArray().Num() != 2)
		{
			Ctx.State.Errors.Add(TEXT("TPair expects an array of size 2, but was %d"), Data->AsArray().Num());
			return false;
		}
		
		Out = TPair<T,S>();

		if (!Ctx.Deserialize(Data->AsArray()[0], Out.Key, VulRuntime::Field::FPathItem(TInPlaceType<int>(), 0)))
		{
			return false;
		}

		if (!Ctx.Deserialize(Data->AsArray()[1], Out.Value, VulRuntime::Field::FPathItem(TInPlaceType<int>(), 1)))
		{
			return false;
		}

		return true;
	}
};

template <>
struct TVulFieldSerializer<FGuid>
{
	static bool Serialize(const FGuid& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (!Value.IsValid())
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}

		Out = MakeShared<FJsonValueString>(Value.ToString());
		return true;
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, FGuid& Out, FVulFieldDeserializationContext& Ctx)
	{
		Out = FGuid();
		
		if (Data->Type == EJson::Null)
		{
			return true;
		}

		if (!Ctx.State.Errors.RequireJsonType(Data, EJson::String))
		{
			return false;
		}

		if (!FGuid::Parse(Data->AsString(), Out))
		{
			Ctx.State.Errors.Add(TEXT("Cannot parse invalid FGuid string `%s`"), *Data->AsString());
			return false;
		}

		return true;
	}
};

