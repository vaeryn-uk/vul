#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulEditorDataTableTestStructs.generated.h"

USTRUCT()
struct FTestStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY()
	int Num;

	UPROPERTY(meta=(VulRowName))
	FName RowName;
};

USTRUCT()
struct FTestCharacter : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(meta=(VulRowName))
	FName Name;

	UPROPERTY()
	int Hp;

	UPROPERTY()
	int Strength;
};

USTRUCT()
struct FTestWeapon : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY()
	int Damage;

	UPROPERTY()
	int MinStrength;
};
