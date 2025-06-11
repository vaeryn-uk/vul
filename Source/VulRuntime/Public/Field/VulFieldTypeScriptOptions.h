#pragma once

#include "CoreMinimal.h"
#include "VulRuntime.h"

/** Options to customize TypeScript definition generation. */
struct VULRUNTIME_API FVulFieldTypeScriptOptions
{
    /**
     * If true, generate exported type guard functions for types with
     * discriminator fields.
     */
    bool GenerateTypeGuardFunctions = false;
};

