#include "Field/FVulFieldSerializationContext.h"

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