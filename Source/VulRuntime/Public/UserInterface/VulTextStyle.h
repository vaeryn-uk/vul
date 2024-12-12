#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CommonTextBlock.h"
#include "RichText/VulRichTextBlock.h"
#include "VulTextStyle.generated.h"

/**
 * Override the text styles to allow them to be edited inline in the editor.
 */
UCLASS(EditInlineNew)
class VULRUNTIME_API UVulTextStyle : public UCommonTextStyle
{
	GENERATED_BODY()

public:
	/**
	 * Convenience function to apply the provided style to a text block widget.
	 *
	 * Returns true if we could apply the style.
	 */
	static bool ApplyTo(const TSoftClassPtr<UVulTextStyle>& Style, UVulRichTextBlock* TextBlock);
};
