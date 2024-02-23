#include "UserInterface/Tooltip/VulTooltipUserWidget.h"

#include "VulRuntimeSettings.h"
#include "UserInterface/Tooltip/VulTooltipSubsystem.h"

UVulTooltipUserWidget::UVulTooltipUserWidget()
{
	// To ensure this can detect events to trigger tooltip.
	UVulTooltipUserWidget::SetVisibility(ESlateVisibility::Visible);
}

FReply UVulTooltipUserWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	auto Ret = Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	const auto Data = GetTooltipData();

	if (!Data)
	{
		return Ret;
	}

	VulRuntime::Tooltip(this)->Show("RichText", GetOwningPlayer(), Data);

	// Don't return a handled reply otherwise that prevents our player controller reporting
	// an accurate mouse position when moving within this widget.
	return Ret;
}

void UVulTooltipUserWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	VulRuntime::Tooltip(this)->Hide("RichText", GetOwningPlayer());
}

TSharedPtr<const FVulTooltipData> UVulTooltipUserWidget::GetTooltipData() const
{
	return nullptr;
}
