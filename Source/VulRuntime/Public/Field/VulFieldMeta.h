#pragma once
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
	void Prop(const FString& Name, const TSharedPtr<FVulFieldDescription>& Description, bool Required);

	/* Scalar type definitions. Sets a simple type. */
	void String() { Type = EJson::String; }
	void Number() { Type = EJson::Number; }
	void Boolean() { Type = EJson::Boolean; }

	void Array(const TSharedPtr<FVulFieldDescription>& ItemsDescription);

	bool Map(
		const TSharedPtr<FVulFieldDescription>& KeysDescription,
		const TSharedPtr<FVulFieldDescription>& ValuesDescription
	);

	TSharedPtr<FJsonValue> JsonSchema() const;

	bool IsValid() const;

private:
	EJson Type = EJson::None;
	TSharedPtr<FVulFieldDescription> Items;
	TMap<FString, TSharedPtr<FVulFieldDescription>> Properties;
	TSharedPtr<FVulFieldDescription> AdditionalProperties;
	TArray<TSharedPtr<FVulFieldDescription>> OneOf;
	TArray<FString> RequiredProperties;
	bool CanBeRef = false;
	TArray<TSharedPtr<FJsonValue>> Enum;
	TOptional<FString> TypeName;
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
	static bool Describe(struct FVulFieldSerializationContext& Ctx, const TSharedPtr<FVulFieldDescription>& Description)
	{
		return false;
	}
};