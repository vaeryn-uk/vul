#include "UserInterface/VulUserInterface.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"

bool VulRuntime::UserInterface::AttachRootUMG(UWidget* Widget, APlayerController* Controller, const int ZOrder)
{
	const auto ViewportSS = UGameViewportSubsystem::Get(Controller->GetWorld());
	if (!ensureAlwaysMsgf(IsValid(ViewportSS), TEXT("Could not get UGameViewportSubsystem to attach UMG widget")))
	{
		return false;
	}

	auto Slot = ViewportSS->GetWidgetSlot(Widget);
	Slot.ZOrder = ZOrder;

	return ViewportSS->AddWidgetForPlayer(Widget, Controller->GetLocalPlayer(), Slot);
}

TOptional<FVector2D> VulRuntime::UserInterface::CalculateScreenPosition(
	UWidget* Widget,
	APlayerController* Controller,
	const FVector& WorldLocation,
	const FVector2D& Offset,
	const FVector2D& Anchor,
	const bool ClampToScreen
) {
	if (Widget->GetDesiredSize().IsNearlyZero())
	{
		// Don't render if the widget isn't reporting a desired size. Stops flickering
		// when first appearing.
		return {};
	}

	FVector2D ActorPos;
	if (!Controller->ProjectWorldLocationToScreen(WorldLocation, ActorPos, true))
	{
		return {};
	}

	FIntVector2 ScreenSize;
	Controller->GetViewportSize(ScreenSize.X, ScreenSize.Y);
	if (ScreenSize == FIntVector2::ZeroValue)
	{
		return {};
	}

	const auto PixelOffset = FVector2D(Offset.X * ScreenSize.X, Offset.Y * ScreenSize.Y);

	auto Result = ActorPos + PixelOffset + AnchorOffset(Widget, Anchor);

	if (ClampToScreen)
	{
		Result = FVector2D::Clamp(
			Result,
			Widget->GetDesiredSize() / 2,
			FVector2D(ScreenSize.X, ScreenSize.Y) - (Widget->GetDesiredSize() / 2)
		);
	}

	return Result;
}

FVector2D VulRuntime::UserInterface::AnchorOffset(UWidget* Widget, const FVector2D& Anchor)
{
	return FVector2D(-Widget->GetDesiredSize().X * Anchor.X, -Widget->GetDesiredSize().Y * Anchor.Y);
}
