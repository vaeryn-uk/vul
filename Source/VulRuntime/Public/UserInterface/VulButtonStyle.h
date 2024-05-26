#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CommonButtonBase.h"
#include "VulButtonStyle.generated.h"

/**
 * Override the button styles to allow them to be edited inline in the editor.
 */
UCLASS(EditInlineNew)
class VULRUNTIME_API UVulButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()
};
