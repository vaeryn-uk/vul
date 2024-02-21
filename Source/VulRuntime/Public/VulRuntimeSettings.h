#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
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
	 * When rendering a tooltip alongside the mouse cursor, how much offset do we apply in screen pixels?
	 *
	 * This avoids the tooltip being obstructed by the mouse cursor.
	 */
	UPROPERTY(EditAnywhere, Config, Category="User Interface|Tooltip")
	FVector2D TooltipMouseOffset = {12, 12};

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
	 * The icons made available to Vul's Rich Text Block.
	 *
	 * TODO: Test this RequiredAssetDataTags specifier.
	 */
	UPROPERTY(EditAnywhere, Config, Category="User Interface|Rich Text", meta=(RequiredAssetDataTags = "RowStructure=/Script/CommonUI.RichTextIconData"))
	TSoftObjectPtr<UDataTable> RichTextIconsTable;


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