#pragma once

#include "CoreMinimal.h"
#include "VulFieldSerializationContext.h"
#include "VulFieldSet.h"
#include "UObject/Object.h"
#include "VulFieldSetObject.generated.h"

UINTERFACE()
class VULRUNTIME_API UVulFieldSetObject : public UInterface
{
	GENERATED_BODY()
};

class VULRUNTIME_API IVulFieldSetObject
{
	GENERATED_BODY()

public:
	virtual FVulFieldSet VulFieldSet() const { return FVulFieldSet(); }
};


// TODO: concepts require C++20. This okay?
template <typename T>
concept IsUObject = std::is_base_of_v<UObject, T>;

template <IsUObject T>
struct TVulFieldSerializer<T*>
{
	static bool Serialize(const T* const& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		if (auto FieldSetObj = Cast<IVulFieldSetObject>(Value); FieldSetObj != nullptr)
		{
			if (!FieldSetObj->VulFieldSet().Serialize(Out, Ctx))
			{
				return false;
			}
		}

		// TODO: Just doing an empty object - not useful? Error?
		Out = MakeShared<FJsonValueObject>(MakeShared<FJsonObject>());
		return true;
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, T*& Out, FVulFieldDeserializationContext& Ctx)
	{
		if (!IsValid(Ctx.ObjectOuter))
		{
			Ctx.Errors.Add(TEXT("Cannot deserialize UObject without a ObjectOuter set"));
			return false;
		}
		
		Out = NewObject<T>(Ctx.ObjectOuter, Out->StaticClass());

		if (auto FieldSetObj = Cast<IVulFieldSetObject>(Out); FieldSetObj != nullptr)
		{
			if (!FieldSetObj->VulFieldSet().Deserialize(Data, Ctx))
			{
				return false;
			}
		}
		
		return true;
	}
};