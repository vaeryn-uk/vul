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
