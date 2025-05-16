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

void FVulFieldDescription::Array(const TSharedPtr<FVulFieldDescription>& ItemsDescription)
{
	ensureMsgf(Type == EJson::Array || Type == EJson::None, TEXT("should not define items as is already non-array type"));

	Type = EJson::Array;
	Items = ItemsDescription; 
}

void FVulFieldDescription::Map(
	const TSharedPtr<FVulFieldDescription>& KeysDescription,
	const TSharedPtr<FVulFieldDescription>& ValuesDescription
) {
	ensureMsgf(Type == EJson::Object || Type == EJson::None, TEXT("should not define map as is already non-object type"));

	ensureMsgf(KeysDescription->Type == EJson::String, TEXT("should not use a non-string type for a field map key"));

	Type = EJson::Object;
	AdditionalProperties = ValuesDescription;
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

	Out->Values.Add("type", MakeShared<FJsonValueString>(VulRuntime::Field::JsonTypeToString(Type)));

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

	return MakeShared<FJsonValueObject>(Out);
}

bool FVulFieldDescription::IsValid() const
{
	return Type != EJson::None;
}
