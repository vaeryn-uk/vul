#pragma once

#include "CoreMinimal.h"
#include "VulTooltip.h"
#include "Components/Widget.h"
#include "Misc/VulContextToggle.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VulTooltipSubsystem.generated.h"

/**
 * Describes how a tooltip is anchor to another element.
 *
 * The absence of this, and the default, is positioning relative to the mouse cursor.
 */
struct FVulTooltipAnchor
{
	FVulTooltipAnchor() = default;
	FVulTooltipAnchor(UWidget* InWidget) : Widget(InWidget) {}

	TWeakObjectPtr<UWidget> Widget;

	bool operator==(const FVulTooltipAnchor& Other) const;
};

/**
 * Manages the Vul tooltip implementation. Responsible for showing and updating tooltip
 * widgets. This will do nothing if Vul tooltip functionality is not configured in settings.
 */
UCLASS()
class VULRUNTIME_API UVulTooltipSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FVulTooltipSubsystem, STATGROUP_Tickables); }
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;

	/**
	 * Makes the tooltip visible for a given player given the provided data.
	 *
	 * Context is required to differentiate different reasons that a tooltip would be made
	 * visible, and a tooltip will be shown until all contexts for that player controller
	 * have requested Hide.
	 *
	 * AnchorWidget can be provided to fix the position of the tooltip to the provided widget.
	 * If omitted, the tooltip will follow the mouse cursor.
	 */
	void Show(
		const FString& Context,
		APlayerController* Controller,
		TSharedPtr<const FVulTooltipData> Data,
		const TOptional<FVulTooltipAnchor>& InAnchor = {}
	) const;

	/**
	 * Hides the tooltip for the given player controller from the given context.
	 *
	 * If there are still outstanding contexts that have shown the tooltip, it will remain visible.
	 */
	void Hide(const FString& Context, const APlayerController* Controller) const;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FVulOnTooltipDataShown, TSharedPtr<const FVulTooltipData>, UUserWidget*)
	DECLARE_MULTICAST_DELEGATE_OneParam(FVulOnTooltipDataHidden, TSharedPtr<const FVulTooltipData>)

	/**
	 * Invoked when new tooltip data is shown. The tooltip data and configured widget will be provided.
	 *
	 * This is called after the widget has been updated with the new data.
	 */
	FVulOnTooltipDataShown OnDataShown;

	/**
	 * Invoked when tooltip data is hidden (including when the tooltip changes to new data). The data that was hidden is provided.
	 */
	FVulOnTooltipDataHidden OnDataHidden;

private:
	struct FWidgetState
	{
		/**
		 * Tracks the contexts from which a tooltip is made visible.
		 *
		 * If no contexts are wanting the tooltip visible, it is hidden.
		 */
		TVulStrCtxToggle Contexts;

		/**
		 * The widget instance.
		 */
		TWeakObjectPtr<UUserWidget> Widget;

		/**
		 * The last-rendered hash of widget content to prevent redraws.
		 *
		 * This is only set whilst the tooltip is shown.
		 */
		TOptional<FString> Hash;

		/**
		 * A reference to the player controller, used for updating the widget position.
		 */
		TWeakObjectPtr<const APlayerController> Controller;

		/**
		 * The last data rendered for this player.
		 */
		TSharedPtr<const FVulTooltipData> Data;

		/**
		 * How to position the tooltip. If unset, positioned relative to the mouse.
		 */
		TOptional<FVulTooltipAnchor> Anchor;
	};

	/**
	 * Resolves the player's widget state entry, creating it if not exists.
	 */
	FWidgetState& GetState(const APlayerController* Controller) const;

	/**
	 * Widget state data, tracked per player (indexed by player index).
	 */
	mutable TArray<FWidgetState> Entries;

	/**
	 * Calculates the tooltip widget's ideal location based on For, moving it as
	 * required to avoid clipping off screen.
	 *
	 * For is the location the widget would appear without adjustment. For example, the
	 * mouse location.
	 *
	 * This sets the location the top left of the widget should appear on the screen.
	 *
	 * Returns false if a position cannot be calculated.
	 */
	bool BestWidgetLocationForMouse(const FVector2D& For, const UWidget* Widget, FVector2D& Out) const;

	/**
	 * Finds the best location when anchoring the tooltip to another widget.
	 * This appears above or below the anchoring widget, avoiding clipping off screen.
	 * The tooltip is positioned centrally to the provided anchor.
	 *
	 * Returns false if a position cannot be calculated.
	 */
	bool BestWidgetLocationForWidget(const UWidget* AnchorWidget, const UWidget* Tooltip, FVector2D& Out);

	/**
	 * Returns the screen space coordinates of the provided widget, at the normalized point on widget
	 * specified by 0-1 anchors (e.g. (0.5, 1) is center X, bottom).
	 */
	FVector2D GetWidgetScreenCoords(const UWidget* Widget, const float AnchorX, const float AnchorY);

	bool bIsEnabled;
};

namespace VulRuntime
{
	/**
	 * Access to the global UVulTooltipSubsystem.
	 */
	VULRUNTIME_API UVulTooltipSubsystem* Tooltip(const UObject* WorldCtx);

	DECLARE_DELEGATE_RetVal(TSharedPtr<const FVulTooltipData>, FVulGetTooltipData);

	/**
	 * Applies Vul tooltip functionality to the provided widget.
	 *
	 * This is useful to add the functionality to native widget types without having to extend
	 * them in your own projects to get at the handlers.
	 */
	VULRUNTIME_API void Tooltipify(const FString& Context, UWidget* Widget, FVulGetTooltipData Getter);
}