﻿#include "Field/VulFieldSerializationContext.h"
#include "VulRuntime.h"
#include "VulRuntimeSettings.h"
#include "Field/VulFieldUtil.h"

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
		Add(TEXT("Required JSON type %s, but got %s"), *JsonTypeToString(Type), *JsonTypeToString(Value->Type));
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
	for (const auto Message : Errors)
	{
		UE_LOG(LogVul, Error, TEXT("FVulField de/serialization error: %s"), *Message);
	}
}

FString FVulFieldSerializationErrors::PathStr() const
{
	return VulRuntime::Field::PathStr(Stack);
}

FString FVulFieldSerializationErrors::JsonTypeToString(EJson Type)
{
	switch (Type)
	{
	case EJson::None: return TEXT("None");
	case EJson::Null: return TEXT("Null");
	case EJson::String: return TEXT("String");
	case EJson::Number: return TEXT("Number");
	case EJson::Boolean: return TEXT("Boolean");
	case EJson::Array: return TEXT("Array");
	case EJson::Object: return TEXT("Object");
	default: return TEXT("Unknown");
	}
}
