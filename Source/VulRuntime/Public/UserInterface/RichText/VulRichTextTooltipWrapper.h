#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UObject/Object.h"
#include "UserInterface/Tooltip/VulTooltip.h"
#include "VulRichTextTooltipWrapper.generated.h"

/**
 * Base widget that is used by Vul's rich text tooltip system.
 *
 * Extend this class in a blueprint and set it in Vul settings.
 */
UCLASS(Abstract)
class UVulRichTextTooltipWrapper : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UVulRichTextTooltipWrapper();

	void VulInit(TSharedPtr<const FVulTooltipData> InTooltipData, const FText& InContent);

	UPROPERTY(meta=(BindWidget))
	class UVulRichTextBlock* Content;

protected:
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	TSharedPtr<const FVulTooltipData> TooltipData;
};
