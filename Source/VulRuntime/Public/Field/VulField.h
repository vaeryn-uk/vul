#pragma once

#include "CoreMinimal.h"
#include "VulFieldSerializer.h"
#include "VulFieldCommonSerializers.h"
#include "VulFieldSerializationContext.h"
#include "UObject/Object.h"

/**
 * A field that can be conveniently serialized/deserialized.
 *
 * This is a wrapper around a pointer that allows Get and Set operations.
 * For the de/serialization itself, TVulFieldSerializer must be implemented
 * for the type you're wrapping.
 *
 * Note the FVulField and associated APIs deal with FJsonValue. This is the chosen
 * portable intermediate representation, though the fields API has been
 * designed to be a more generic de/serialization toolkit than just strictly JSON.
 *
 * When describing your types' fields, you'll likely want a VulFieldSet.
 */
struct VULRUNTIME_API FVulField
{
	/**
	 * Define a field that can serialized and deserialized.
	 */
	template <typename T>
	static FVulField Create(T* Ptr)
	{
		FVulField Out;
		Out.Ptr = Ptr;
		
		Out.Read = [](
			void* Ptr,
			TSharedPtr<FJsonValue>& Out,
			FVulFieldSerializationContext& Ctx,
			const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
		) {
			return Ctx.Serialize<T>(*static_cast<T*>(Ptr), Out, IdentifierCtx);
		};
		
		Out.Write = [](
			const TSharedPtr<FJsonValue>& Value,
			void* Ptr,
			FVulFieldDeserializationContext& Ctx,
			const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
		) {
			return Ctx.Deserialize<T>(Value, *static_cast<T*>(Ptr), IdentifierCtx);
		};
		
		return Out;
	}
	
	/**
	 * Define a field that can only be serialized. This field will be ignored when deserializing
	 * from a field set, and direct attempts to deserialize this will fail.
	 */
	template <typename T>
	static FVulField Create(const T& Ptr)
	{
		FVulField Out;
		Out.bIsReadOnly = true;
		Out.Ptr = const_cast<T*>(&Ptr); // const cast to satisfy Ptr. We won't ever write to this, however.
		
		Out.Read = [](
			void* Ptr,
			TSharedPtr<FJsonValue>& Out,
			FVulFieldSerializationContext& Ctx,
			const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
		) {
			return Ctx.Serialize<T>(*reinterpret_cast<T*>(Ptr), Out, IdentifierCtx);
		};
		
		Out.Write = [](
			const TSharedPtr<FJsonValue>& Value,
			void* Ptr,
			FVulFieldDeserializationContext& Ctx,
			const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
		) {
			Ctx.State.Errors.Add(TEXT("cannot write read-only field"));
			return false;
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

	bool Deserialize(const TSharedPtr<FJsonValue>& Value);
	bool Deserialize(
		const TSharedPtr<FJsonValue>& Value,
		FVulFieldDeserializationContext& Ctx,
		const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx = {}
	);
	
	bool Serialize(TSharedPtr<FJsonValue>& Out) const;
	bool Serialize(
		TSharedPtr<FJsonValue>& Out,
		FVulFieldSerializationContext& Ctx,
		const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx = {}
	) const;

	template <typename CharType = TCHAR>
	bool DeserializeFromJson(const FString& JsonStr, FVulFieldDeserializationContext& Ctx)
	{
		TSharedPtr<FJsonValue> ParsedJson;
		auto Reader = TJsonReaderFactory<CharType>::Create(JsonStr);
		if (!FJsonSerializer::Deserialize(Reader, ParsedJson) || !ParsedJson.IsValid())
		{
			Ctx.State.Errors.Add(TEXT("cannot parse invalid JSON string"));
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
			Ctx.State.Errors.Add(TEXT("serialization of JSON string failed"));
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

	bool IsReadOnly() const;

private:
	bool bIsReadOnly = false;
	void* Ptr = nullptr;
	
	TFunction<bool (
		void*,
		TSharedPtr<FJsonValue>&,
		FVulFieldSerializationContext& Ctx,
		const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
	)> Read;
	
	TFunction<bool (
		const TSharedPtr<FJsonValue>&,
		void*,
		FVulFieldDeserializationContext& Ctx,
		const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
	)> Write;
};

template <typename T>
concept HasVulField = requires(const T& Obj) {
	{ Obj.VulField() } -> std::same_as<struct FVulField>;
};

/**
 * Serialization support for types that define a `FVulField VulField() const` function.
 */
template<HasVulField T>
struct TVulFieldSerializer<T>
{
	static bool Serialize(const T& Value, TSharedPtr<FJsonValue>& Out, struct FVulFieldSerializationContext& Ctx)
	{
		return Value.VulField().Serialize(Out, Ctx);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out, struct FVulFieldDeserializationContext& Ctx)
	{
		return Out.VulField().Deserialize(Data, Ctx);
	}
};