#pragma once

#include "CoreMinimal.h"
#include "Field/VulField.h"
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

	FVulFieldSet FieldSet()
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