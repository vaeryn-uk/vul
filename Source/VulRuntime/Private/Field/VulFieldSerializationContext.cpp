#include "Field/VulFieldSerializationContext.h"
#include "VulRuntime.h"
#include "VulRuntimeSettings.h"
#include "Field/VulFieldUtil.h"
#include "Field/VulFieldRegistry.h"

bool FVulFieldSerializationErrors::IsSuccess() const
{
	return Errors.IsEmpty();
}

void FVulFieldSerializationErrors::SetMaxStack(int N)
{
	MaxStackSize = N;
}

bool FVulFieldSerializationErrors::RequireJsonType(const TSharedPtr<FJsonValue>& Value, const EJson Type)
{
	if (Value->Type != Type)
	{
		Add(
			TEXT("Required JSON type %s, but got %s"),
			*VulRuntime::Field::JsonTypeToString(Type),
			*VulRuntime::Field::JsonTypeToString(Value->Type)
		);
		
		return false;
	}

	return true;
}

bool FVulFieldSerializationErrors::RequireJsonProperty(
	const TSharedPtr<FJsonValue>& Value,
	const FString& Property,
	TSharedPtr<FJsonValue>& Out,
	const TOptional<EJson> Type
) {
	if (!RequireJsonType(Value, EJson::Object))
	{
		return false;
	}

	if (!Value->AsObject()->Values.Contains(Property))
	{
		Add(TEXT("Required JSON property `%s` is not defined"), *Property);
		return false;
	}

	if (Type.IsSet() && !RequireJsonType(Value->AsObject()->Values[Property], Type.GetValue()))
	{
		return false;
	}

	Out = Value->AsObject()->Values[Property];
	return true;
}

VulRuntime::Field::FPath FVulFieldSerializationErrors::GetPath() const
{
	return Stack;
}

void FVulFieldSerializationErrors::Push(const VulRuntime::Field::FPathItem& Identifier)
{
	Stack.Add(Identifier);
}

void FVulFieldSerializationErrors::Pop()
{
	if (!Stack.IsEmpty())
	{
		Stack.RemoveAt(Stack.Num() - 1);
	}
}

bool FVulFieldSerializationErrors::WithIdentifierCtx(
	const TOptional<VulRuntime::Field::FPathItem>& Identifier,
	const TFunction<bool()>& Fn
) {
	if (Identifier.IsSet())
	{
		Push(Identifier.GetValue());
	}
	
	if (Stack.Num() > MaxStackSize)
	{
		Add(TEXT("Maximum stack size %d. Infinite recursion?"), MaxStackSize);
		return false;
	}
	
	const auto Ret = Fn();
	
	if (Identifier.IsSet())
	{
		Pop();
	}
	
	return Ret;
}

void FVulFieldSerializationErrors::Log()
{
	for (const auto& Message : Errors)
	{
		UE_LOG(LogVul, Error, TEXT("FVulField de/serialization error: %s"), *Message);
	}
}

FString FVulFieldSerializationErrors::PathStr() const
{
	return VulRuntime::Field::PathStr(Stack);
}

TOptional<FString> FVulFieldSerializationContext::KnownTypeName(const FString& TypeId)
{
	const auto Entry = FVulFieldRegistry::Get().GetType(TypeId);

	if (Entry.IsSet())
	{
		return Entry->Name;
	}

	return {};
}

bool FVulFieldSerializationContext::IsBaseType(const FString& TypeId)
{
	return FVulFieldRegistry::Get().GetSubtypes(TypeId).Num() > 0;
}

bool FVulFieldSerializationContext::GenerateBaseTypeDescription(
	const FString& TypeId,
	const TSharedPtr<FVulFieldDescription>& Description
) {
	TArray<TSharedPtr<FVulFieldDescription>> Subtypes;

	const auto DiscField = FVulFieldRegistry::Get().GetType(TypeId)->DiscriminatorField;

	for (const auto& Entry : FVulFieldRegistry::Get().GetSubtypes(TypeId))
	{
		TSharedPtr<FVulFieldDescription> SubDesc = MakeShared<FVulFieldDescription>();

		if (!Entry.DescribeFn(*this, SubDesc))
		{
			State.Errors.Add(TEXT("Failed to describe subtype %s"), *Entry.Name);
			return false;
		}

		if (DiscField.IsSet() && Entry.DiscriminatorValue.IsSet())
		{
			if (const auto DiscriminatorDesc = SubDesc->GetProperty(DiscField.GetValue()))
			{
				const auto Discriminator = MakeShared<FVulFieldDescription>();

				Discriminator->Const(
					MakeShared<FJsonValueString>(Entry.DiscriminatorValue.GetValue()()),
					DiscriminatorDesc
				);

				SubDesc->Prop(DiscField.GetValue(), Discriminator, true);
			}
		}

		Subtypes.Add(SubDesc);
	}

	if (!Subtypes.IsEmpty())
	{
		Description->Union(Subtypes);
	}

	return true;
}
