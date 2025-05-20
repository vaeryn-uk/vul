#include "Field/VulFieldMeta.h"
#include "Field/VulFieldRegistry.h"
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

bool FVulFieldDescription::Const(const TSharedPtr<FJsonValue>& Value)
{
	const static TArray Allowed = {EJson::Number, EJson::String, EJson::Boolean};

	if (!ensureAlwaysMsgf(
		Allowed.Contains(Value->Type),
		TEXT("Cannot use Const() with a complex value type (must be str, bool or number)")
	))
	{
		return false;
	}

	ConstValue = Value;

	return true;
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
	TSharedPtr<FJsonObject> Definitions = MakeShared<FJsonObject>();

	auto Out = JsonSchema(Definitions, true);

	if (!Definitions->Values.IsEmpty())
	{
		Out->AsObject()->Values.Add("definitions", MakeShared<FJsonValueObject>(Definitions));
	}

	return Out;
}

bool FVulFieldDescription::IsValid() const
{
	return Type != EJson::None || !UnionTypes.IsEmpty() || TypeId.IsSet() || ConstValue.IsValid();
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

FString FVulFieldDescription::TypeScriptDefinitions() const
{
	TArray<TSharedPtr<FVulFieldDescription>> Descriptions;

	const static FString Indent = "\t";

	FString Out;
	
	GetNamedTypes(Descriptions);

	for (const auto Description : Descriptions)
	{
		const auto TypeName = Description->GetTypeName().GetValue();
		
		if (!Description->EnumValues.IsEmpty())
		{
			Out += FString::Printf(TEXT("export enum %s {"), *TypeName);
			Out += LINE_TERMINATOR;

			for (const auto Value : Description->EnumValues)
			{
				if (!ensureAlwaysMsgf(Value->Type == EJson::String, TEXT("Only string enum values are supported (%s)"), *TypeName))
				{
					continue;
				}
				
				Out += Indent + Value->AsString() + " = \"" + Value->AsString() + "\",";
				Out += LINE_TERMINATOR;
			}
			
			Out += "}";
			Out += LINE_TERMINATOR;
			Out += LINE_TERMINATOR;
		} else if (!Description->Properties.IsEmpty())
		{
			Out += FString::Printf(TEXT("export type %s = {"), *TypeName);
			Out += LINE_TERMINATOR;

			for (const auto Entry : Description->Properties)
			{
				Out += Indent + Entry.Key + ": " + Entry.Value->TypeScriptType() + ";";
				Out += LINE_TERMINATOR;
			}

			Out += "}";
			Out += LINE_TERMINATOR;
			Out += LINE_TERMINATOR;
		}
	}

	return Out;
}

void FVulFieldDescription::GetNamedTypes(TArray<TSharedPtr<FVulFieldDescription>>& Types) const
{
	if (TypeId.IsSet())
	{
		const auto AlreadyExists = Types.IndexOfByPredicate([&](const TSharedPtr<FVulFieldDescription>& Existing)
		{
			return Existing->TypeId == TypeId;
		});

		if (AlreadyExists != INDEX_NONE)
		{
			return;
		}
		
		Types.Add(MakeShared<FVulFieldDescription>(*this));
	}

	for (const auto Prop : Properties)
	{
		Prop.Value->GetNamedTypes(Types);
	}

	for (const auto Subtype : UnionTypes)
	{
		Subtype->GetNamedTypes(Types);
	}

	if (Items.IsValid())
	{
		Items->GetNamedTypes(Types);
	}
}

TOptional<FString> FVulFieldDescription::GetTypeName() const
{
	if (TypeId.IsSet())
	{
		return FVulFieldRegistry::Get().GetType(TypeId.GetValue())->Name;
	}

	return TOptional<FString>();
}

TSharedPtr<FJsonValue> FVulFieldDescription::JsonSchema(const TSharedPtr<FJsonObject>& Definitions, const bool AddToDefinitions) const
{
	if (!IsValid())
	{
		// If no useful description has been provided, we want to match
		// anything, which is bool(true) in JSON schema.
		return MakeShared<FJsonValueBoolean>(true);
	}

	TOptional<FString> TypeName = {};
	TSharedPtr<FJsonValueObject> RefObject;

	if (TypeId.IsSet())
	{
		auto Entry = FVulFieldRegistry::Get().GetType(TypeId.GetValue());
		TypeName = Entry->Name;
		
		RefObject = MakeShared<FJsonValueObject>(MakeShared<FJsonObject>(TMap<FString, TSharedPtr<FJsonValue>>{
			{"$ref", MakeShared<FJsonValueString>(FString::Printf(TEXT("#definitions/%s"), *TypeName.GetValue()))}
		}));;

		if (Definitions->Values.Contains(TypeName.GetValue()))
		{
			return RefObject;
		}
	}
	
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();

	if (AddToDefinitions && TypeName.IsSet())
	{
		Definitions->Values.Add(TypeName.GetValue(), MakeShared<FJsonValueObject>(Out));
	}

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
			ChildProperties->Values.Add(Child.Key, Child.Value->JsonSchema(Definitions));
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
		Out->Values.Add("items", Items->JsonSchema(Definitions));
	}

	if (AdditionalProperties.IsValid())
	{
		Out->Values.Add("additionalProperties", AdditionalProperties->JsonSchema(Definitions));
	}

	if (!UnionTypes.IsEmpty())
	{
		TArray<TSharedPtr<FJsonValue>> OneOf;
		
		for (const auto Subtype : UnionTypes)
		{
			OneOf.Add(Subtype->JsonSchema(Definitions));
		}
		
		Out->Values.Add("oneOf", MakeShared<FJsonValueArray>(OneOf));
	}

	if (!EnumValues.IsEmpty())
	{
		Out->Values.Add("enum", MakeShared<FJsonValueArray>(EnumValues));
	}

	if (ConstValue.IsValid())
	{
		Out->Values.Add("const", ConstValue);
	}

	if (TypeName.IsSet())
	{
		Out->Values.Add("x-vul-typename", MakeShared<FJsonValueString>(TypeName.GetValue()));
	}

	if (RefObject.IsValid())
	{
		return RefObject;
	}

	return MakeShared<FJsonValueObject>(Out);
}

FString FVulFieldDescription::TypeScriptType() const
{
	const auto KnownType = GetTypeName();
	if (KnownType.IsSet())
	{
		return KnownType.GetValue();
	}

	if (Type == EJson::String)
	{
		return "string";
	}

	if (Type == EJson::Boolean)
	{
		return "boolean";
	}

	if (Type == EJson::Number)
	{
		return "number";
	}

	if (Items.IsValid())
	{
		return Items->TypeScriptType() + "[]";
	}

	return "any";
}
