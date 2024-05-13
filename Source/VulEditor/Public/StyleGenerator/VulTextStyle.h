#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CommonTextBlock.h"
#include "VulTextStyle.generated.h"

/**
 * Override the text styles to allow them to be edited inline in the editor.
 */
UCLASS(EditInlineNew)
class VULEDITOR_API UVulTextStyle : public UCommonTextStyle
{
	GENERATED_BODY()
};
