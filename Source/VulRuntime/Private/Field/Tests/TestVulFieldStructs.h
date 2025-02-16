#pragma once

#include "CoreMinimal.h"
#include "Field/VulField.h"
#include "Field/VulFieldSet.h"
#include "UObject/Object.h"
#include "TestVulFieldStructs.generated.h"

USTRUCT()
struct FVulTestFieldType
{
	GENERATED_BODY()
	
	bool B;
	int I;
	FString S;
	TMap<FString, int> M;
	TArray<bool> A;

	FVulFieldSet FieldSet() const
	{
		FVulFieldSet Out;

		Out.Add(FVulField::Create(&B), "bool");
		Out.Add(FVulField::Create(&I), "int");
		Out.Add(FVulField::Create(&S), "string");
		Out.Add(FVulField::Create(&M), "map");
		Out.Add(FVulField::Create(&A), "array");

		return Out;
	}
};


USTRUCT()
struct FVulTestFieldParent
{
	GENERATED_BODY()
	
	FVulTestFieldType Inner;
	
	FVulFieldSet FieldSet() const
	{
		FVulFieldSet Out;

		Out.Add(FVulField::Create(&Inner), "inner");

		return Out;
	}
};

template<>
struct FVulFieldSerializer<FVulTestFieldType>
{
	static bool Serialize(const FVulTestFieldType& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		return Value.FieldSet().Serialize(Out, Ctx);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, FVulTestFieldType& Out, FVulFieldDeserializationContext& Ctx)
	{
		return Out.FieldSet().Deserialize(Data, Ctx);
	}
};