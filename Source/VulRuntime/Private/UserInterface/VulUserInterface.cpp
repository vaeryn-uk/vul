#include "UserInterface/VulUserInterface.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"

bool VulRuntime::UserInterface::AttachRootUMG(UWidget* Widget, APlayerController* Controller, const int ZOrder)
{
	const auto ViewportSS = UGameViewportSubsystem::Get();
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
		const FVector2D SizeInScreenPixels = Widget->GetDesiredSize() * UWidgetLayoutLibrary::GetViewportScale(Widget);
		Result = FVector2D::Clamp(
			Result,
			SizeInScreenPixels / 2,
			FVector2D(ScreenSize.X, ScreenSize.Y) - (SizeInScreenPixels / 2)
		);
	}

	return Result;
}

TOptional<FVector2D> VulRuntime::UserInterface::CalculateScreenPosition(
	UWidget* Widget,
	APlayerController* Controller,
	const FVector2D& Position,
	const FVector2D& Anchor,
	const bool ClampToScreen
) {
	FIntVector2 ScreenSize;
	Controller->GetViewportSize(ScreenSize.X, ScreenSize.Y);
	if (ScreenSize == FIntVector2::ZeroValue)
	{
		return {};
	}

	auto Result = FVector2D(Position.X * ScreenSize.X, Position.Y * ScreenSize.Y) + AnchorOffset(Widget, Anchor);

	if (ClampToScreen)
	{
		const FVector2D SizeInScreenPixels = Widget->GetDesiredSize() * UWidgetLayoutLibrary::GetViewportScale(Widget);
		Result = FVector2D::Clamp(
			Result,
			SizeInScreenPixels / 2,
			FVector2D(ScreenSize.X, ScreenSize.Y) - (SizeInScreenPixels / 2)
		);
	}

	return Result;
}

FVector2D VulRuntime::UserInterface::AnchorOffset(UWidget* Widget, const FVector2D& Anchor)
{
	// GetDesiredSize() is in Slate's DPI-independent local units; multiplying by the viewport scale converts it
	// into raw screen pixels.
	const FVector2D SizeInScreenPixels = Widget->GetDesiredSize() * UWidgetLayoutLibrary::GetViewportScale(Widget);
	return FVector2D(-SizeInScreenPixels.X * Anchor.X, -SizeInScreenPixels.Y * Anchor.Y);
}
