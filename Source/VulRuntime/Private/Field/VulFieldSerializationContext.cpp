#include "Field/VulFieldSerializationContext.h"

bool FVulFieldSerializationErrors::IsSuccess() const
{
	return Errors.IsEmpty();
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
