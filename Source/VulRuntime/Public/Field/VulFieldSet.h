﻿#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Field/VulField.h"
#include "VulFieldSet.generated.h"

/**
 * A collection of FVulFields that can be de/serialized.
 *
 * This is designed to allow your types to expose a field set that describes its
 * data, then we can easily de/serialize instances of that type at all at once.
 *
 * This is akin to an Object in JSON.
 */
struct VULRUNTIME_API FVulFieldSet
{
	struct VULRUNTIME_API FEntry
	{
		/**
		 * When serializing, this property will be included even if its value is empty.
		 *
		 * The default behaviour is to omit empty values (checked via VulRuntime::Field::IsEmpty).
		 */
		FEntry& EvenIfEmpty(const bool IncludeIfEmpty = true);

		TOptional<FString> GetTypeId() const;
	private:
		friend FVulFieldSet;
		FVulField Field;
		bool OmitIfEmpty = true;
		
		TFunction<bool (
			TSharedPtr<FJsonValue>&,
			FVulFieldSerializationContext&,
			const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
		)> Fn = nullptr;
		
		TFunction<bool (
			FVulFieldSerializationContext& Ctx,
			TSharedPtr<FVulFieldDescription>& Description,
			const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
		)> Describe = nullptr;

		TOptional<FString> TypeId;
	};
	
	/**
	 * Adds a field to the set. If read only, the field will only be
	 * serialized, but ignored for deserialization.
	 *
	 * Set IsRef=true to have this field be the value used when using FVulField's
	 * shared reference system.
	 */
	FEntry& Add(const FVulField& Field, const FString& Identifier, const bool IsRef = false);

	/**
	 * Adds a virtual field - one whose value is derived from a function
	 * call. These are only relevant when serializing a field set.
	 *
	 * This is useful for adding additional data to your serialized outputs,
	 * where the data is supplemental and not required to reconstitute an
	 * object correctly.
	 *
	 * Set IsRef=true to have this field be the value used when using FVulField's
	 * shared reference system.
	 */
	template <typename T>
	FEntry& Add(TFunction<T ()> Fn, const FString& Identifier, const bool IsRef = false)
	{
		FEntry Created;
		
		Created.Fn = [Fn](
			TSharedPtr<FJsonValue>& Out,
			FVulFieldSerializationContext& Ctx,
			const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
		) {
			return Ctx.Serialize<T>(Fn(), Out, IdentifierCtx);
		};
		
		if (IsRef)
		{
			RefField = Identifier;
		}

		Created.Describe = [](
			FVulFieldSerializationContext& Ctx,
			TSharedPtr<FVulFieldDescription>& Description,
			const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx = {}
		) {
			return Ctx.Describe<T>(Description, IdentifierCtx);
		};

		if (FVulFieldRegistry::Get().Has<T>())
		{
			Created.TypeId = FVulFieldRegistry::Get().GetType<T>()->TypeId;
		}

		return Entries.Add(Identifier, Created);
	}

	TSharedPtr<FJsonValue> GetRef(FVulFieldSerializationState& State) const;
	bool HasRef() const;

	/**
	 * Defines a validity function for this field set. This Fn must return
	 * true to indicate the field set's data and should be serialized as normal.
	 * False indicates the data is invalid and should be serialized as null
	 * (and then omitted, if the outer field's OmitIfEmpty is true).
	 *
	 * If no ValidityFn is defined, the default behaviour is that field sets are always
	 * valid (not nullable).
	 */
	void ValidityFn(const TFunction<bool ()>& Fn);
	
	bool IsValid() const;
	bool CanBeInvalid() const;

	bool Serialize(TSharedPtr<FJsonValue>& Out) const;
	bool Serialize(TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx) const;
	bool Deserialize(const TSharedPtr<FJsonValue>& Data);
	bool Deserialize(const TSharedPtr<FJsonValue>& Data, FVulFieldDeserializationContext& Ctx);
	
	template <typename CharType = TCHAR, typename PrintPolicy = TCondensedJsonPrintPolicy<CharType>>
	bool SerializeToJson(FString& Out, FVulFieldSerializationContext& Ctx) const
	{
		TSharedPtr<FJsonValue> Json;
		if (!Serialize(Json, Ctx))
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
		
		return Deserialize(ParsedJson, Ctx);
	}
	
	template <typename CharType = TCHAR>
	bool DeserializeFromJson(const FString& JsonStr)
	{
		FVulFieldDeserializationContext Ctx;
		return DeserializeFromJson<CharType>(JsonStr, Ctx);
	}

	bool Describe(FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description) const;
private:
	TMap<FString, FEntry> Entries;
	TOptional<FString> RefField = {};
	TFunction<bool ()> IsValidFn = nullptr;
};


UINTERFACE()
class VULRUNTIME_API UVulFieldSetAware : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface on your UObjects to make them compatible with the FVulField
 * serialization & deserialization system.
 */
class VULRUNTIME_API IVulFieldSetAware
{
	GENERATED_BODY()

public:
	virtual FVulFieldSet VulFieldSet() const { return FVulFieldSet(); }
};

template <typename T>
concept HasVulFieldSet = std::is_base_of_v<IVulFieldSetAware, T> || 
	requires(const T& Obj) {
		{ Obj.VulFieldSet() } -> std::same_as<FVulFieldSet>;
	};

template <HasVulFieldSet T>
struct TVulFieldSerializer<T>
{
	static bool Serialize(const T& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		return Value.VulFieldSet().Serialize(Out, Ctx);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out, FVulFieldDeserializationContext& Ctx)
	{
		auto Result = Out.VulFieldSet().Deserialize(Data, Ctx);
		if (!Result)
		{
			return false;
		}
		return Result;
	}
};

template <typename T>
concept IsUObject = std::is_base_of_v<UObject, T>;

template <HasVulFieldSet T>
struct TVulFieldMeta<T>
{
	static bool Describe(FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
	{
		FVulFieldSet Set;
		if constexpr (IsUObject<T>)
		{
			T* Default = NewObject<T>();
			Set = Default->VulFieldSet();
		} else
		{
			T Default;
			Set = Default.VulFieldSet();
		}

		if (Set.CanBeInvalid())
		{
			Description->Nullable();
		}

		return Set.Describe(Ctx, Description);
	}
};

template<HasVulFieldSet T>
struct TVulFieldRefResolver<T>
{
	static bool SupportsRef()
	{
		FVulFieldSet Set;
		
		if constexpr (IsUObject<T>)
		{
			T* Default = NewObject<T>();
			Set = Default->VulFieldSet();
		} else
		{
			T Default;
			Set = Default.VulFieldSet();
		}

		return Set.HasRef();
	}
	
	static bool Resolve(
		const T& Value,
		TSharedPtr<FJsonValue>& Out,
		FVulFieldSerializationState& State
	) {
		Out = Value.VulFieldSet().GetRef(State);
		return Out.IsValid();
	}
};

template<>
struct TVulFieldRefResolver<UObject>
{
	// Special case: UObject ref resolution needs to report itself as supported,
	// even though actual UObjects cannot resolve references alone. This is
	// required due to how TScriptInterface deserialization uses UObjects, then
	// at runtime uses UE reflection to resolve to the correct derived UObject type.
	
	static bool SupportsRef() { return true; }
	
	static bool Resolve(
		const UObject& Value,
		TSharedPtr<FJsonValue>& Out,
		FVulFieldSerializationState& State
	) {
		return false;
	}
};

template <IsUObject T>
struct TVulFieldSerializer<T*>
{
	static bool Serialize(const T* const& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (!IsValid(Value))
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}
		
		if (auto FieldSetObj = Cast<IVulFieldSetAware>(Value); FieldSetObj != nullptr)
		{
			return FieldSetObj->VulFieldSet().Serialize(Out, Ctx);
		}
		
		if (Ctx.Flags.IsEnabled(VulFieldSerializationFlag_AssetReferencing, Ctx.State.Errors.GetPath()) && Value->IsAsset())
		{
			const auto Path = FSoftObjectPath(Value);
			Out = MakeShared<FJsonValueString>(Path.ToString());
			return true;
		}

		// TODO: Just doing an empty object - not useful? Error?
		Out = MakeShared<FJsonValueObject>(MakeShared<FJsonObject>());
		return true;
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T*& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (Data->Type == EJson::Null)
		{
			Out = nullptr;
			return true;
		}
		
		if (Ctx.Flags.IsEnabled(VulFieldSerializationFlag_AssetReferencing, Ctx.State.Errors.GetPath()))
		{
			FString AsStr;
			if (Data->TryGetString(AsStr) && FSoftObjectPath(AsStr).IsValid())
			{
				Out = LoadObject<T>(Ctx.ObjectOuter, *AsStr);
				return true;
			}
		}
		
		Out = NewObject<T>(Ctx.ObjectOuter, Out->StaticClass());

		if (auto FieldSetObj = Cast<IVulFieldSetAware>(Out); FieldSetObj != nullptr)
		{
			return FieldSetObj->VulFieldSet().Deserialize(Data, Ctx);
		}
		
		return true;
	}
};

template<typename T>
struct TVulFieldSerializer<TScriptInterface<T>>
{
	static bool Serialize(const TScriptInterface<T>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		return Ctx.Serialize(Value.GetObject(), Out);
	}
	
	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TScriptInterface<T>& Out, FVulFieldDeserializationContext& Ctx)
	{
		UObject* Obj;
		if (!Ctx.Deserialize(Data, Obj))
		{
			return false;
		}

		if (Cast<T>(Obj) == nullptr)
		{
			Ctx.State.Errors.Add(TEXT("deserialized object of class which does not implement the expected interface"));
			return false;
		}

		Out = TScriptInterface<T>(Obj);
		return true;
	}
};

template <typename T>
struct TVulFieldMeta<TScriptInterface<T>>
{
	static bool Describe(FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
	{
		// We don't expect any field integration with interfaces beyond their registered
		// types so the rest of the meta system can describe their concrete implementations.
		if (!FVulFieldRegistry::Get().Has<T>())
		{
			Ctx.State.Errors.Add(TEXT("Cannot describe UINTERFACE as it has not been registered with VULFLD_ macros"));
			return false;
		}

		if (bool _; !Ctx.RegisterDescription<T>(Description, _))
		{
			return false;
		}

		return true;
	}
};