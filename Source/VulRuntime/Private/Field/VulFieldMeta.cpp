#include "Field/VulFieldMeta.h"
#include "Field/VulFieldUtil.h"

void FVulFieldDescription::Prop(const FString& Name, const TSharedPtr<FVulFieldDescription>& Description, bool Required)
{
	ensureMsgf(Type == EJson::Object || Type == EJson::None, TEXT("should not add property `%s` as is already non-object type"), *Name);
	
	Type = EJson::Object;
	Properties.Add(Name, Description);

	if (Required)
	{
		RequiredProperties.Add(Name);
	}
}

void FVulFieldDescription::Union(const TArray<TSharedPtr<FVulFieldDescription>>& Subtypes)
{
	TArray<FVulFieldDescription> UniqueTypes;
	
	for (const auto Subtype : Subtypes)
	{
		if (!UniqueTypes.Contains(*Subtype.Get()))
		{
			UniqueTypes.Add(*Subtype.Get());
		}
	}

	if (UniqueTypes.Num() == 1)
	{
		*this = UniqueTypes[0];
		return;
	}

	UnionTypes = Subtypes;
}

void FVulFieldDescription::Array(const TSharedPtr<FVulFieldDescription>& ItemsDescription)
{
	ensureMsgf(Type == EJson::Array || Type == EJson::None, TEXT("should not define items as is already non-array type"));

	Type = EJson::Array;
	Items = ItemsDescription;
}

void FVulFieldDescription::Enum(const FString& Item)
{
	String();
	
	EnumValues.Add(MakeShared<FJsonValueString>(Item));
}

bool FVulFieldDescription::Map(
	const TSharedPtr<FVulFieldDescription>& KeysDescription,
	const TSharedPtr<FVulFieldDescription>& ValuesDescription
) {
	ensureMsgf(Type == EJson::Object || Type == EJson::None, TEXT("should not define map as is already non-object type"));

	if (!ensureMsgf(KeysDescription->Type == EJson::String, TEXT("should not use a non-string type for a field map key")))
	{
		return false;
	}

	Type = EJson::Object;
	AdditionalProperties = ValuesDescription;

	return true;
}

TSharedPtr<FJsonValue> FVulFieldDescription::JsonSchema() const
{
	if (!IsValid())
	{
		// If no useful description has been provided, we want to match
		// anything, which is bool(true) in JSON schema.
		return MakeShared<FJsonValueBoolean>(true);
	}
	
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();

	if (Type != EJson::None)
	{
		TSharedPtr<FJsonValue> TypeNode;
		
		if (IsNullable)
		{
			TypeNode = MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{
				MakeShared<FJsonValueString>(VulRuntime::Field::JsonTypeToString(Type)),
				MakeShared<FJsonValueString>(VulRuntime::Field::JsonTypeToString(EJson::Null)),
			});
		} else
		{
			TypeNode = MakeShared<FJsonValueString>(VulRuntime::Field::JsonTypeToString(Type));
		}
		
		Out->Values.Add("type", TypeNode);
	}

	if (!Properties.IsEmpty())
	{
		ensureAlwaysMsgf(Type == EJson::Object, TEXT("Cannot describe properties of a VulField that is not an object"));
		
		TSharedPtr<FJsonObject> ChildProperties = MakeShared<FJsonObject>();
		
		for (const auto Child : Properties)
		{
			ChildProperties->Values.Add(Child.Key, Child.Value->JsonSchema());
		}
		
		Out->Values.Add("properties", MakeShared<FJsonValueObject>(ChildProperties));
		
		TArray<TSharedPtr<FJsonValue>> RequiredProps;
		for (const auto Prop : RequiredProperties)
		{
			RequiredProps.Add(MakeShared<FJsonValueString>(Prop));
		}
		
		if (!RequiredProps.IsEmpty())
		{
			Out->Values.Add("required", MakeShared<FJsonValueArray>(RequiredProps));
		}
	}

	if (Items.IsValid())
	{
		Out->Values.Add("items", Items->JsonSchema());
	}

	if (AdditionalProperties.IsValid())
	{
		Out->Values.Add("additionalProperties", AdditionalProperties->JsonSchema());
	}

	if (!UnionTypes.IsEmpty())
	{
		TArray<TSharedPtr<FJsonValue>> OneOf;
		
		for (const auto Subtype : UnionTypes)
		{
			OneOf.Add(Subtype->JsonSchema());
		}
		
		Out->Values.Add("oneOf", MakeShared<FJsonValueArray>(OneOf));
	}

	if (!EnumValues.IsEmpty())
	{
		Out->Values.Add("enum", MakeShared<FJsonValueArray>(EnumValues));
	}

	return MakeShared<FJsonValueObject>(Out);
}

bool FVulFieldDescription::IsValid() const
{
	return Type != EJson::None || !UnionTypes.IsEmpty();
}

bool FVulFieldDescription::operator==(const FVulFieldDescription& Other) const
{
	if (Type == Other.Type)
	{
		if (Type == EJson::String || Type == EJson::Number || Type == EJson::Boolean)
		{
			return IsNullable == Other.IsNullable;
		}
	}

	// TODO: Only simple types can be equal for now. Implement something more thorough.

	return false;
}
