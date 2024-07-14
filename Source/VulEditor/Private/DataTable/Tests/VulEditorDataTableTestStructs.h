#pragma once

#include "CoreMinimal.h"
#include "DataTable/VulDataRepository.h"
#include "UObject/Object.h"
#include "VulEditorDataTableTestStructs.generated.h"

USTRUCT()
struct FTestStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY()
	int Num = 0;

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
	int Hp = 0;

	UPROPERTY()
	int Strength = 0;
};

USTRUCT()
struct FTestWeapon : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY()
	int Damage = 0;

	UPROPERTY()
	int MinStrength = 0;
};

USTRUCT()
struct FTestDataPtr : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY()
	FVulDataPtr Ptr;
};