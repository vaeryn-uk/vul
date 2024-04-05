#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Components/SizeBox.h"
#include "Framework/Text/ITextDecorator.h"
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

/**
 * Implement this in inline widgets that will have their size controlled by the Vul rich text system.
 *
 * The default is to have the widget size match the surrounding text, but this can be scaled by:
 *  - implementing GetAutoSizeDefaultScale in your widget.
 *  - a scale="SOME_FLOAT" in the markup.
 *
 * Both scaling sources are combined.
 */
class IVulAutoSizedInlineWidget
{
public:
	virtual ~IVulAutoSizedInlineWidget() = default;

	/**
	 * Sizes the widget given the inline text rendering info, if the widget supports it.
	 */
	static void ApplyAutoSizing(UWidget* Widget, const FTextRunInfo& RunInfo, const FTextBlockStyle& TextStyle);

	static float RecommendedHeight(const FTextBlockStyle& TextStyle);

	/**
	 * When the rich text system adds this widget in line in text, this SizeBox (if not null)
	 * will have its max height override set based on the surrounding rich text.
	 */
	virtual USizeBox* GetAutoSizeBox() { return nullptr; }

	/**
	 * When applying an automatic size to GetAutoSizeBox, optionally return a proportion
	 * here that indicates the width of the widget based on the calculated height.
	 *
	 * An unset return leaves this as the default.
	 */
	virtual TOptional<float> GetAutoSizeAspectRatio() { return {}; }

	/**
	 * Implement this to scale the widget by some constant factor.
	 */
	virtual float GetAutoSizeDefaultScale() { return 1.f; }

	/**
	 * When applying scaling, should we ensure that the widget is rendered vertically aligned with the
	 * surrounding text?
	 */
	virtual bool AutoSizeVerticallyCentre() { return true; }
};