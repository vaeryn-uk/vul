#include "UserInterface/RichText/VulRichTextTooltipWrapper.h"
#include "UserInterface/RichText/VulRichTextBlock.h"
#include "UserInterface/Tooltip/VulTooltipSubsystem.h"

UVulRichTextTooltipWrapper::UVulRichTextTooltipWrapper()
{
	// To ensure this can detect events to trigger tooltip.
	UVulRichTextTooltipWrapper::SetVisibility(ESlateVisibility::Visible);
}

void UVulRichTextTooltipWrapper::VulInit(TSharedPtr<const FVulTooltipData> InTooltipData, const FText& InContent)
{
	Content->SetText(InContent);
	TooltipData = InTooltipData;
}

FReply UVulRichTextTooltipWrapper::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	auto Ret = Super::NativeOnMouseMove(InGeometry, InMouseEvent);

	if (!TooltipData.IsValid())
	{
		return Ret;
	}

	VulRuntime::Tooltip(this)->Show("RichText", GetOwningPlayer(), TooltipData);

	// Don't return a handled reply otherwise that prevents our player controller reporting
	// an accurate mouse position when moving within this widget.
	return Ret;
}

void UVulRichTextTooltipWrapper::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (!TooltipData.IsValid())
	{
		return;
	}

	VulRuntime::Tooltip(this)->Hide("RichText", GetOwningPlayer());
}
