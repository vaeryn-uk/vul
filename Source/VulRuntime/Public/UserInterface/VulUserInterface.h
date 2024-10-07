#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UObject/Object.h"

namespace VulRuntime::UserInterface
{
	/**
	 * Attaches the provided UMG widget to the player's viewport at the given Z order.
	 */
	bool VULRUNTIME_API AttachRootUMG(UWidget* Widget, APlayerController* Controller, const int ZOrder = 0);

	/**
	 * Calculates the screen position of the provided widget that places it on top of the provided world location.
	 *
	 * Offset is a value between 0-1 of screen space to adjust the position, e.g. (0, .1) will return
	 * a position 10% above the actor.
	 *
	 * Anchor controls how the widget is positioned based on its size, e.g. (0, 0) would return the widget
	 * positioned with its top-left corner as the return, and (.5, .5) is a centered widget position.
	 *
	 * Returns unset if a position cannot be found.
	 */
	TOptional<FVector2D> VULRUNTIME_API CalculateScreenPosition(
		UWidget* Widget,
		APlayerController* Controller,
		const FVector& WorldLocation,
		const FVector2D& Offset = FVector2D(0, 0),
		const FVector2D& Anchor = FVector2D(.5f, .5f)
	);
}
