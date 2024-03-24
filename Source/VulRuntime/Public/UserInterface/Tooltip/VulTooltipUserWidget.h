#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "VulTooltip.h"
#include "VulTooltipUserWidget.generated.h"

/**
 * Provides a base implementation for a user widget that displays a tooltip when hovered.
 *
 * Alternatively consider VulRuntime::Tooltipify if you don't want to extend this.
 */
UCLASS()
class VULRUNTIME_API UVulTooltipUserWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UVulTooltipUserWidget();

protected:
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/**
	 * Optionally return tooltip data to have a tooltip triggered when this widget is hovered.
	 *
	 * Default is to return nullptr (so no tooltip is shown).
	 */
	virtual TSharedPtr<const FVulTooltipData> GetTooltipData() const;
};
