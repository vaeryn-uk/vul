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

TSharedPtr<FVulFieldDescription> FVulFieldDescription::GetProperty(const FString& Name)
{
	if (Properties.Contains(Name))
	{
		return Properties[Name];
	}

	return nullptr;
}

bool FVulFieldDescription::Const(const TSharedPtr<FJsonValue>& Value, const TSharedPtr<FVulFieldDescription>& Of)
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
	ConstOf = Of;

	return true;
}

bool FVulFieldDescription::AreEquivalent(
	const TSharedPtr<FVulFieldDescription>& A,
	const TSharedPtr<FVulFieldDescription>& B
) {
	if (!A.IsValid() && !B.IsValid()) return true;
	if (A.IsValid() != B.IsValid()) return false;

	// Recursion protection: if this field is a known type and is not customized
	// with const value restrictions, consider equal.
	if (A->TypeId.IsSet() && A->TypeId == B->TypeId && !A->ConstValue.IsValid() && !B->ConstValue.IsValid())
	{
		return true;
	}
	
	if (A->TypeId != B->TypeId) return false;
	if (A->Type != B->Type) return false;
	if (A->IsNullable != B->IsNullable) return false;
	if (A->Referencing != B->Referencing) return false;
	if (!AreEquivalent(A->Items, B->Items)) return false;
	if (!AreEquivalent(A->AdditionalProperties, B->AdditionalProperties)) return false;
	if (!AreEquivalent(A->ConstOf, B->ConstOf)) return false;

	if (A->Properties.Num() != B->Properties.Num()) return false;
	for (const auto& Prop : A->Properties)
	{
		if (!B->Properties.Contains(Prop.Key) || !AreEquivalent(Prop.Value, B->Properties[Prop.Key]))
		{
			return false;
		}
	}

	if (A->ConstValue.IsValid() != B->ConstValue.IsValid()) return false;
	if (A->ConstValue && !FJsonValue::CompareEqual(*A->ConstValue.Get(), *B->ConstValue.Get())) return false;
	
	if (A->EnumValues.Num() != B->EnumValues.Num()) return false;
	for (int I = 0; I < A->EnumValues.Num(); I++)
	{
		if (!FJsonValue::CompareEqual(*A->EnumValues[I], *B->EnumValues[I]))
		{
			return false;
		}
	}
	
	if (A->UnionTypes.Num() != B->UnionTypes.Num()) return false;
	for (int I = 0; I < A->UnionTypes.Num(); I++)
	{
		if (!AreEquivalent(A->UnionTypes[I], B->UnionTypes[I]))
		{
			return false;
		}
	}

	return true;
}

TSharedPtr<FVulFieldDescription> FVulFieldDescription::CreateVulRef()
{
	TSharedPtr<FVulFieldDescription> Out = MakeShared<FVulFieldDescription>();

	Out->String();
	Out->Documentation = "A string reference to another object in the graph.";

	return Out;
}

void FVulFieldDescription::Union(const TArray<TSharedPtr<FVulFieldDescription>>& Subtypes)
{
	TArray<FVulFieldDescription> UniqueTypes;
	
	for (const auto& Subtype : Subtypes)
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

bool FVulFieldDescription::IsObject() const
{
	return Type == EJson::Object;
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

bool FVulFieldDescription::HasEnumValue(const FString& Item) const
{
	for (const auto& Value : EnumValues)
	{
		if (Value->AsString() == Item)
		{
			return true;
		}
	}

	return false;
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

	TSharedPtr<FJsonValue> Out = JsonSchema(Definitions, true);

	if (ContainsReference(EReferencing::Reference))
	{
		TSharedPtr<FJsonValue> Refs = MakeShared<FJsonValueObject>(MakeShared<FJsonObject>(TMap<FString, TSharedPtr<FJsonValue>>{
			{"type", MakeShared<FJsonValueString>("object")},
		}));
		
		Out = MakeShared<FJsonValueObject>(MakeShared<FJsonObject>(TMap<FString, TSharedPtr<FJsonValue>>{
			{"refs", Refs},
			{"data", Out},
		}));

		Out = MakeShared<FJsonValueObject>(MakeShared<FJsonObject>(TMap<FString, TSharedPtr<FJsonValue>>{
			{"type", MakeShared<FJsonValueString>("object")},
			{"properties", Out},
		}));
	}

	if (!Definitions->Values.IsEmpty())
	{
		Out->AsObject()->Values.Add("definitions", MakeShared<FJsonValueObject>(Definitions));

		if (MayContainReference())
		{
			Definitions->Values.Add("VulFieldRef", CreateVulRef()->JsonSchema());
		}
	}

	return Out;
}

bool FVulFieldDescription::IsValid() const
{
	return Type != EJson::None || !UnionTypes.IsEmpty() || TypeId.IsSet() || ConstValue.IsValid();
}

bool FVulFieldDescription::operator==(const FVulFieldDescription& Other) const
{
	return AreEquivalent(MakeShared<FVulFieldDescription>(*this), MakeShared<FVulFieldDescription>(Other));
}

FString FVulFieldDescription::TypeScriptDefinitions(const FVulFieldTypeScriptOptions& Options) const
{
	TMap<FString, TSharedPtr<FVulFieldDescription>> Descriptions;

	const static FString Indent = "\t";
	const static FString LineEnding = "\n";

	FString Out;

	if (MayContainReference())
	{
		Out += "// A string reference to an existing object of the given type" + LineEnding;
		Out += "// @ts-ignore" + LineEnding;
		Out += "export type VulFieldRef<T> = string;" + LineEnding;
		Out += LineEnding;
	}

	if (ContainsReference(EReferencing::Reference))
	{
		Out += "export type VulRefs = Record<VulFieldRef<any>, any>;" + LineEnding;
		Out += LineEnding;
	}
	
	GetNamedTypes(Descriptions);

	// Deterministic order: alphabetical on type name.
	Descriptions.ValueSort([](const TSharedPtr<FVulFieldDescription>& A, const TSharedPtr<FVulFieldDescription>& B)
	{
		return A->GetTypeName().Get("") < B->GetTypeName().Get("");
	});

	for (const auto& Entry : Descriptions)
	{
		const auto Description = Entry.Value;
		const auto TypeName = Description->GetTypeName().GetValue();
		
		if (!Description->EnumValues.IsEmpty())
		{
			Out += FString::Printf(TEXT("export enum %s {"), *TypeName);
			Out += LineEnding;

			for (const auto& Value : Description->EnumValues)
			{
				if (!ensureAlwaysMsgf(Value->Type == EJson::String, TEXT("Only string enum values are supported (%s)"), *TypeName))
				{
					continue;
				}
				
				Out += Indent + Value->AsString() + " = \"" + Value->AsString() + "\",";
				Out += LineEnding;
			}
			
			Out += "}";
			Out += LineEnding;
			Out += LineEnding;
		} else if (!Description->Properties.IsEmpty() || Entry.Value->UnionTypes.Num() > 0)
		{
			const auto BaseType = FVulFieldRegistry::Get().GetBaseType(Entry.Key);
			TSharedPtr<FVulFieldDescription> BaseDesc;
			if (BaseType.IsSet())
			{
				BaseDesc = Descriptions.Contains(BaseType->TypeId) ? Descriptions[BaseType->TypeId] : nullptr;
			}
			
			if (BaseDesc.IsValid())
			{
				Out += FString::Printf(
					TEXT("export interface %s extends %s {"),
					*TypeName,
					*BaseDesc->GetTypeName().GetValue()
				);
			} else
			{
				Out += FString::Printf(TEXT("export interface %s {"), *TypeName);
			}
			
			Out += LineEnding;

			for (const auto& PropertyEntry : Description->Properties)
			{
				if (BaseDesc.IsValid())
				{
					const auto BaseProp = BaseDesc->GetProperty(PropertyEntry.Key);
					if (BaseProp.IsValid() && AreEquivalent(BaseProp, PropertyEntry.Value))
					{
						// Skip duplicate properties as their presence in the
						// base type implies the property on subtypes.
						continue;
					}
				}

				const auto Separator = Description->IsPropertyRequired(PropertyEntry.Key) ? ": " : "?: ";
				Out += Indent + PropertyEntry.Key + Separator + PropertyEntry.Value->TypeScriptType() + ";";
				Out += LineEnding;
			}

			Out += "}";
			Out += LineEnding;
			Out += LineEnding;

			if (BaseType.IsSet() && Options.DiscriminatorTypeGuardFunctions)
			{
				const auto RegistryEntry = FVulFieldRegistry::Get().GetType(Entry.Key);
				
				if (
					RegistryEntry.IsSet() &&
					BaseType->DiscriminatorField.IsSet() &&
					RegistryEntry->DiscriminatorValue.IsSet()
				) {
					const auto DiscriminatorField = BaseType->DiscriminatorField.GetValue();
					const auto DiscriminatorValue = RegistryEntry->DiscriminatorValue.GetValue()();

					Out += FString::Printf(
						TEXT("export function is%s(object: any): object is %s {"),
						*TypeName,
						*TypeName
					);
					Out += LineEnding;
					Out += Indent + FString::Printf(
						TEXT("return object.%s === \"%s\";"),
						*DiscriminatorField,
						*DiscriminatorValue
					);
					Out += LineEnding;
					Out += "}";
					Out += LineEnding;
					Out += LineEnding;
				}
			}
		} else if (TArray{EJson::String, EJson::Number, EJson::Boolean}.Contains(Entry.Value->Type))
		{
			// Simple type alias.
			Out += FString::Printf(
				TEXT("export type %s = %s;"),
				*Entry.Value->GetTypeName().GetValue(),
				*Entry.Value->TypeScriptType(false)
			);
			Out += LineEnding;
			Out += LineEnding;
		}
	}

	return Out;
}

void FVulFieldDescription::GetNamedTypes(TMap<FString, TSharedPtr<FVulFieldDescription>>& Types) const
{
	if (TypeId.IsSet())
	{
		if (Types.Contains(TypeId.GetValue()))
		{
			return;
		}

		Types.Add(TypeId.GetValue(), MakeShared<FVulFieldDescription>(*this));
	}

	for (const auto& Prop : Properties)
	{
		Prop.Value->GetNamedTypes(Types);
	}

	for (const auto& Subtype : UnionTypes)
	{
		Subtype->GetNamedTypes(Types);
	}

	if (Items.IsValid())
	{
		Items->GetNamedTypes(Types);
	}

	if (AdditionalProperties.IsValid())
	{
		AdditionalProperties->GetNamedTypes(Types);
	}
}

void FVulFieldDescription::UniqueDescriptions(
	const FVulFieldDescription* Description,
	TArray<const FVulFieldDescription*>& Descriptions
) {
	if (Descriptions.Contains(Description))
	{
		return;
	}

	Descriptions.Add(Description);

	for (const auto& Prop : Description->Properties)
	{
		UniqueDescriptions(Prop.Value.Get(), Descriptions);
	}

	for (const auto& Subtype : Description->UnionTypes)
	{
		UniqueDescriptions(Subtype.Get(), Descriptions);
	}

	if (Description->Items.IsValid())
	{
		UniqueDescriptions(Description->Items.Get(), Descriptions);
	}

	if (Description->AdditionalProperties.IsValid())
	{
		UniqueDescriptions(Description->AdditionalProperties.Get(), Descriptions);
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

bool FVulFieldDescription::ContainsReference(const EReferencing Ref) const
{
	TArray<const FVulFieldDescription*> Entries;
	UniqueDescriptions(this, Entries);

	for (const auto& Entry : Entries)
	{
		if (Entry->Referencing == Ref)
		{
			return true;
		}
	}

	return false;
}

bool FVulFieldDescription::MayContainReference() const
{
	TArray<const FVulFieldDescription*> Entries;
	UniqueDescriptions(this, Entries);

	for (const auto& Entry : Entries)
	{
		if (Entry->Referencing != EReferencing::None)
		{
			return true;
		}
	}

	return false;
}

bool FVulFieldDescription::IsPropertyRequired(const FString& Prop) const
{
	return RequiredProperties.Contains(Prop);
}

TSharedPtr<FJsonValue> FVulFieldDescription::JsonSchema(
	const TSharedPtr<FJsonObject>& Definitions,
	const bool AddToDefinitions
) const {
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
		}));

		if (Referencing != EReferencing::None)
		{
			TSharedPtr<FJsonObject> VulFieldRef = MakeShared<FJsonObject>(TMap<FString, TSharedPtr<FJsonValue>>{
				{"$ref", MakeShared<FJsonValueString>("#definitions/VulFieldRef")}
			});

			if (Referencing == EReferencing::Possible)
			{
				TArray<TSharedPtr<FJsonValue>> OneOfs = {
					RefObject,
					MakeShared<FJsonValueObject>(VulFieldRef)
				};
				
				RefObject = MakeShared<FJsonValueObject>(MakeShared<FJsonObject>(TMap<FString, TSharedPtr<FJsonValue>>{
					{
						"oneOf",
						MakeShared<FJsonValueArray>(OneOfs),
					}
				}));
			} else if (Referencing == EReferencing::Reference)
			{
				RefObject = MakeShared<FJsonValueObject>(VulFieldRef);
			}
		}

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
		
		for (const auto& Child : Properties)
		{
			ChildProperties->Values.Add(Child.Key, Child.Value->JsonSchema(Definitions));
		}
		
		Out->Values.Add("properties", MakeShared<FJsonValueObject>(ChildProperties));
		
		TArray<TSharedPtr<FJsonValue>> RequiredProps;
		for (const auto& Prop : RequiredProperties)
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
		
		for (const auto& Subtype : UnionTypes)
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

	if (Documentation.IsSet())
	{
		Out->Values.Add("description", MakeShared<FJsonValueString>(Documentation.GetValue()));
	}

	if (RefObject.IsValid())
	{
		return RefObject;
	}

	return MakeShared<FJsonValueObject>(Out);
}

FString FVulFieldDescription::TypeScriptType(const bool AllowRegisteredType) const
{
	const auto KnownType = GetTypeName();
	if (AllowRegisteredType && KnownType.IsSet())
	{
		if (Referencing == EReferencing::Possible)
		{
			return FString::Printf(TEXT("(%s | VulFieldRef<%s>)"), *KnownType.GetValue(), *KnownType.GetValue());
		}

		if (Referencing == EReferencing::Reference)
		{
			return FString::Printf(TEXT("VulFieldRef<%s>"), *KnownType.GetValue());
		}
		
		return KnownType.GetValue();
	}

	if (ConstValue.IsValid())
	{
		if (ConstOf.IsValid() && ConstOf->HasEnumValue(ConstValue->AsString()) && ConstOf->TypeId.IsSet())
		{
			return ConstOf->GetTypeName().GetValue() + "." + ConstValue->AsString();
		}
		
		return VulRuntime::Field::JsonToString(ConstValue);
	}

	if (AdditionalProperties.IsValid())
	{
		return "Record<string, " + AdditionalProperties->TypeScriptType() + ">";
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
