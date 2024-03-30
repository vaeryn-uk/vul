#pragma once

#include "CoreMinimal.h"
#include "Components/HorizontalBox.h"
#include "UObject/Object.h"
#include "Components/VerticalBox.h"
#include "VulElementSpacer.generated.h"

/**
 * Describes a space between two widgets in a horizontal or vertical box container.
 *
 * Use this as a UPROPERTY in a widget and use this to spawn elements in a container.
 * Useful to apply space without needing to assume which container is used.
 */
USTRUCT()
struct VULRUNTIME_API FVulElementSpacer
{
	GENERATED_BODY()

	/**
	 * The space in pixels to apply between elements in a container.
	 *
	 * Elements will be centered, sharing the spacing value equally on top/bottom or left/right.
	 */
	UPROPERTY(EditAnywhere)
	float Spacing = 0;

	/**
	 * Adds Element to a horizontal or vertical box container, applying the spacing setting.
	 */
	UPanelSlot* AddToContainer(UPanelWidget* Container, UWidget* Element) const;

	/**
	 * Adds Element to a horizontal box container, applying the spacing setting.
	 */
	UHorizontalBoxSlot* AddToContainer(UHorizontalBox* Container, UWidget* Element) const;

	/**
	 * Adds Element to a horizontal box container, applying the spacing setting.
	 */
	UVerticalBoxSlot* AddToContainer(UVerticalBox* Container, UWidget* Element) const;
};
