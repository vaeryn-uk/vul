#include "UserInterface/RichText/VulRichTextTooltipWrapper.h"

#include "Components/SizeBoxSlot.h"
#include "Fonts/FontMeasure.h"
#include "Misc/DefaultValueHelper.h"
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

float IVulAutoSizedInlineWidget::RecommendedHeight(const FTextBlockStyle& TextStyle)
{
	const FSlateFontInfo Font = TextStyle.Font;
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure.Get().GetMaxCharacterHeight(Font);
}

void IVulAutoSizedInlineWidget::ApplyAutoSizing(
	UWidget* Widget,
	const FTextRunInfo& RunInfo,
	const FTextBlockStyle& TextStyle
) {
	const auto AutoSized = dynamic_cast<IVulAutoSizedInlineWidget*>(Widget);
	if (AutoSized == nullptr)
	{
		return;
	}

	auto CustomScale = 1.f;
	if (RunInfo.MetaData.Contains("scale"))
	{
		FDefaultValueHelper::ParseFloat(RunInfo.MetaData["scale"], CustomScale);
	}

	if (const auto SizeBox = AutoSized->GetAutoSizeBox(); IsValid(SizeBox))
	{
		// Keep the size box the same height as the widget to ensure this flows
		// with the text in terms of layout. We then mess with negative padding
		// of the contents to center it, breaking out of the laid-out size.
		const auto TextHeight = RecommendedHeight(TextStyle);
		const auto WidgetHeight = TextHeight * AutoSized->GetAutoSizeDefaultScale() * CustomScale;
		SizeBox->SetHeightOverride(TextHeight);

		if (const auto WidthRatio = AutoSized->GetAutoSizeAspectRatio(); WidthRatio.IsSet())
		{
			// Also override width too if the widget requests it.
			SizeBox->SetWidthOverride(WidgetHeight * WidthRatio.GetValue());
		}

		// If the widget requests it, apply some translation to vertically center it.
		// Not sure that this is the best way to do achieve this (via render transform),
		// but some testing suggests that this render translation is respected for mouse
		// hover detection and layouts around the widget.
		if (AutoSized->AutoSizeVerticallyCentre())
		{
			const auto Correction = (WidgetHeight - TextHeight) / 2;
			const auto Slot = Cast<USizeBoxSlot>(SizeBox->GetContentSlot());
			Slot->SetPadding(Slot->GetPadding() + FMargin(0, -Correction, 0, -Correction));
		}
	}
}
