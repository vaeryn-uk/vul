#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulDataTableRow.generated.h"

/**
 *
 */
USTRUCT()
struct VULRUNTIME_API FVulDataTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/**
	 * The identifier for this row.
	 */
	FName VulRowName;
};
