#pragma once

#include "Field/VulFieldUtil.h"
#include "VulFieldSerializationContext.h"

/**
 * Describes a Vul serializable field.
 *
 * Used in metadata operations for representing the structure of serialized data.
 *
 * This API is designed for simple description definitions in TVulFieldMeta type
 * specializations, so users can define their field structures concisely as
 * possible.
 *
 * This borrows heavily from JSON schema as this is the designed-for use: generating
 * JSON schema from Vul field structures.
 */
struct VULRUNTIME_API FVulFieldDescription
{
	/**
	 * Defines a property in this field. Implies an object type.
	 *
	 * Required=true if this property is always present (even if empty).
	 */
	void Prop(const FString& Name, const TSharedPtr<FVulFieldDescription>& Description, bool Required);
	TSharedPtr<FVulFieldDescription> GetProperty(const FString& Name);

	/* Scalar type definitions. Sets a simple type. */
	void String() { Type = EJson::String; }
	void Number() { Type = EJson::Number; }
	void Boolean() { Type = EJson::Boolean; }

	/**
	 * For a field that can only ever be a single value.
	 *
	 * Of specifies the type that this value belongs to. Often an enum description.
	 */
	bool Const(const TSharedPtr<FJsonValue>& Value, const TSharedPtr<FVulFieldDescription>& Of);

	bool static AreEquivalent(const TSharedPtr<FVulFieldDescription>& A, const TSharedPtr<FVulFieldDescription>& B);

	/**
	 * Binds this description to a CPP type that is registered in FVulRegistry.
	 *
	 * This implies the type is reusable throughout a schema.
	 */
	template <typename T>
	void BindToType() { TypeId = VulRuntime::Field::TypeId<T>(); }

	/**
	 * Indicates this field can be null.
	 */
	void Nullable() { IsNullable = true; }

	static TSharedPtr<FVulFieldDescription> CreateVulRef();

	/**
	 * For fields that can be one type or another (or more).
	 *
	 * This will merge in intelligently, detecting if all subtypes are
	 * equivalent and setting this the single common type description
	 * if so.
	 */
	void Union(const TArray<TSharedPtr<FVulFieldDescription>>& Subtypes);

	void MaybeRef() { CanBeRef = true; }

	void Array(const TSharedPtr<FVulFieldDescription>& ItemsDescription);

	/**
	 * Add the given string as one of the allowed values. Can be called repeatedly.
	 */
	void Enum(const FString& Item);

	bool HasEnumValue(const FString& Item) const;

	bool Map(
		const TSharedPtr<FVulFieldDescription>& KeysDescription,
		const TSharedPtr<FVulFieldDescription>& ValuesDescription
	);

	TSharedPtr<FJsonValue> JsonSchema(const bool ExtractRefs = false) const;

	bool IsValid() const;

	bool operator==(const FVulFieldDescription& Other) const;

	TOptional<FString> GetTypeId() const { return TypeId; }

	FString TypeScriptDefinitions(const bool ExtractRefs = false) const;

	/**
	 * Extracts all descriptions that are named types, i.e. registered with FVulFieldRegistry.
	 */
	void GetNamedTypes(TMap<FString, TSharedPtr<FVulFieldDescription>>& Types) const;

	TOptional<FString> GetTypeName() const;

	/**
	 * True if any of part of the field description contains a Vul field ref.
	 */
	bool ContainsReference() const;

private:
	TSharedPtr<FJsonValue> JsonSchema(
		const TSharedPtr<FJsonObject>& Definitions,
		const bool ExtractRefs,
		const bool AddToDefinitions = true
	) const;

	FString TypeScriptType(const bool ExtractRefs, const bool AllowRegisteredType = true) const;
	
	EJson Type = EJson::None;
	TSharedPtr<FVulFieldDescription> Items;
	TMap<FString, TSharedPtr<FVulFieldDescription>> Properties;
	TSharedPtr<FVulFieldDescription> AdditionalProperties;
	TArray<FString> RequiredProperties;
	bool CanBeRef = false;
	TArray<TSharedPtr<FJsonValue>> EnumValues;
	bool IsNullable = false;
	TArray<TSharedPtr<FVulFieldDescription>> UnionTypes = {};
	TSharedPtr<FJsonValue> ConstValue = nullptr;
	TSharedPtr<FVulFieldDescription> ConstOf = nullptr;
	TOptional<FString> TypeId;
	TOptional<FString> Documentation = {};
};

/**
 * Types that want to supply meta information about their Vul serialized forms should specialize this.
 */
template <typename T>
struct TVulFieldMeta
{
	/**
	 * Describes the serialized form of a type, configuring the provided Description.
	 *
	 * Ctx is provided for further, nested serialization and inspecting any serialization
	 * options that may affect the possible formats of this type's serialized form.
	 */
	static bool Describe(struct FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
	{
		return false;
	}
};