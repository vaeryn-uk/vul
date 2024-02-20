﻿#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulRuntimeSettings.generated.h"

/**
 * Global settings for the Vul runtime plugin.
 */
UCLASS(Config=Game)
class VULRUNTIME_API UVulRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	bool IsTooltipEnabled() const;

	virtual FName GetCategoryName() const override { return FName("Game"); }
	virtual FName GetContainerName() const override { return FName("Project"); };

#if WITH_EDITOR
	virtual FText GetSectionDescription() const override { return NSLOCTEXT("FVulRuntime", "SettingsDescription", "Configure Vul Runtime settings."); }
	virtual FText GetSectionText() const override { return NSLOCTEXT("FVulRuntime", "SettingsText", "Vaeryn's Unreal Library Runtime"); }
#endif

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
};

namespace VulRuntime
{
	VULRUNTIME_API const UVulRuntimeSettings* Settings();
}