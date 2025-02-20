#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Field/VulField.h"
#include "VulFieldSet.generated.h"

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
	 *
	 * Set IsRef=true to have this field be the value used when using FVulField's
	 * shared reference system.
	 */
	void Add(const FVulField& Field, const FString& Identifier, const bool IsRef = false);

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
	void Add(TFunction<T ()> Fn, const FString& Identifier, const bool IsRef = false)
	{
		Fns.Add(
			Identifier,
			[Fn](
				TSharedPtr<FJsonValue>& Out,
				FVulFieldSerializationContext& Ctx,
				const TOptional<FString>& IdentifierCtx
			) {
				return Ctx.Serialize<T>(Fn(), Out, IdentifierCtx);
			}
		);
		
		if (IsRef)
		{
			RefField = Identifier;
		}
	}

	TSharedPtr<FJsonValue> GetRef() const;

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
private:
	TMap<FString, FVulField> Fields;
	TOptional<FString> RefField = {};
	
	TMap<FString, TFunction<bool (
			TSharedPtr<FJsonValue>&,
			FVulFieldSerializationContext&,
			const TOptional<FString>& IdentifierCtx = {}
		)>> Fns;
};


UINTERFACE()
class VULRUNTIME_API UVulFieldSetAware : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface on your UObjects to make them compatible with the FVulField
 * serialization & deserialization system.
 *
 * TODO: Should this interface be required 
 */
class VULRUNTIME_API IVulFieldSetAware
{
	GENERATED_BODY()

public:
	virtual FVulFieldSet VulFieldSet() const { return FVulFieldSet(); }
};

// TODO: concepts require C++20. This okay?
template <typename T>
concept HasVulFieldSet = std::is_base_of_v<IVulFieldSetAware, T>;

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

template<HasVulFieldSet T>
struct TVulFieldRefResolver<T>
{
	static constexpr bool SupportsRef = true;
	
	static bool Resolve(const T& Value, TSharedPtr<FJsonValue>& Out)
	{
		Out = Value.VulFieldSet().GetRef();
		return Out.IsValid();
	}
};

// TODO: concepts require C++20. This okay?
template <typename T>
concept IsUObject = std::is_base_of_v<UObject, T>;

template <IsUObject T>
struct TVulFieldSerializer<T*>
{
	static bool Serialize(const T* const& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (auto FieldSetObj = Cast<IVulFieldSetAware>(Value); FieldSetObj != nullptr)
		{
			return FieldSetObj->VulFieldSet().Serialize(Out, Ctx);
		}

		// TODO: Just doing an empty object - not useful? Error?
		Out = MakeShared<FJsonValueObject>(MakeShared<FJsonObject>());
		return true;
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T*& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!IsValid(Ctx.ObjectOuter))
		{
			Ctx.Errors.Add(TEXT("Cannot deserialize UObject without a ObjectOuter set"));
			return false;
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
			Ctx.Errors.Add(TEXT("deserialized object of class which does not implement the expected interface"));
			return false;
		}

		Out = TScriptInterface<T>(Obj);
		return true;
	}
};