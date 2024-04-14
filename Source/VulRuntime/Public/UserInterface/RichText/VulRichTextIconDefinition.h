#pragma once

#include "CoreMinimal.h"
#include "CommonUIRichTextData.h"
#include "UObject/Object.h"
#include "VulRichTextIconDefinition.generated.h"

/**
 * Extends rich text icon data with additional visualization options
 * offered by the Vul icon decorator.
 *
 * Create a data table with these icons, then set this table in Vul
 * project settings.
 */
USTRUCT()
struct VULRUNTIME_API FVulRichTextIconDefinition : public FRichTextIconData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere)
	bool ShowBackground = false;

	UPROPERTY(EditAnywhere)
	FSlateBrush BackgroundBrush;

	UPROPERTY(EditAnywhere)
	FMargin BackgroundPadding = FMargin(0);
};
