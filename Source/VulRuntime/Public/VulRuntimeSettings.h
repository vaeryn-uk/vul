#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UserInterface/RichText/VulRichTextIcon.h"
#include "UserInterface/RichText/VulRichTextTooltipWrapper.h"
#include "VulRuntimeSettings.generated.h"

/**
 * Global settings for the Vul runtime plugin.
 */
UCLASS(Config=Game, DefaultConfig)
class VULRUNTIME_API UVulRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	bool IsTooltipEnabled() const;

	/**
	 * The widget class that we show as the tooltip.
	 *
	 * This must implement IVulTooltipWidget.
	 *
	 * Required to enable Vul's tooltip functionality.
	 */
	UPROPERTY(Config, EditAnywhere, Category="User Interface|Tooltip")
	TSoftClassPtr<UUserWidget> TooltipWidget;

	/**
	 * When rendering a tooltip alongside an element such as the mouse cursor, how much offset do we apply in screen pixels?
	 *
	 * This avoids the tooltip being obstructed.
	 */
	UPROPERTY(EditAnywhere, Config, Category="User Interface|Tooltip")
	FVector2D TooltipOffset = {12, 12};

	/**
	 * The ZOrder value to draw the tooltip. A higher value ensures this appears over other UI elements.
	 */
	UPROPERTY(EditAnywhere, Config, Category="User Interface|Tooltip")
	int32 TooltipZOrder = 100;

	/**
	 * Your widget blueprint that the Vul rich text system will use to display text and/or content
	 * inline in text that will trigger a tooltip when hovered.
	 *
	 * Note this is not the tooltip widget itself.
	 */
	UPROPERTY(EditAnywhere, Config, Category="User Interface|Rich Text")
	TSoftClassPtr<UVulRichTextTooltipWrapper> RichTextTooltipWrapper;

	/**
	 * The icon widget we use to render icons in rich text.
	 *
	 * This is an extension/replacement of CommonUI's icon rich text support to facilitate custom
	 * scaling, as well as control in extending the icon widget if you would like to.
	 *
	 * Rich text syntax: <vi i="icon_row_name" scale="custom_scale"/>
	 *
	 * Where icon_row_name matches a row name in the CommonUI's rich text icons table (see CommonUI settings).
	 * custom_scale can be used to change the scale of this single icon as required. Can be greater than 1,
	 * unlike common UI.
	 */
	UPROPERTY(EditAnywhere, Config, Category="User Interface|Rich Text")
	TSoftClassPtr<UVulRichTextIcon> IconWidget = UVulRichTextIcon::StaticClass();

	/**
	 * Enhanced icon support for rich text when using `UVulRichTextBlock`.
	 *
	 * <vi i=\"row-name\"></> will render the icon in the table matching that row name.
	 */
	UPROPERTY(EditAnywhere, Config, Category="User Interface|Rich Text")
	TSoftObjectPtr<UDataTable> IconSet;

// Editor integration
	virtual FName GetCategoryName() const override { return FName("Game"); }
	virtual FName GetContainerName() const override { return FName("Project"); };

#if WITH_EDITOR
	virtual FText GetSectionDescription() const override { return NSLOCTEXT("FVulRuntime", "SettingsDescription", "Configure Vul Runtime settings."); }
	virtual FText GetSectionText() const override { return NSLOCTEXT("FVulRuntime", "SettingsText", "Vaeryn's Unreal Library Runtime"); }
#endif
// End editor integration
};

namespace VulRuntime
{
	VULRUNTIME_API const UVulRuntimeSettings* Settings();
}