#pragma once

#include "CoreMinimal.h"
#include "VulTooptip.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VulTooltipSubsystem.generated.h"

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
	 */
	void Show(const FString& Context, APlayerController* Controller, TSharedPtr<const FVulTooltipData> Data) const;

	/**
	 * Hides the tooltip for the given player controller from the given context.
	 *
	 * If there are still outstanding contexts that have shown the tooltip, it will remain visible.
	 */
	void Hide(const FString& Context, const APlayerController* Controller) const;

private:
	struct FWidgetState
	{
		/**
		 * Tracks the contexts from which a tooltip is made visible.
		 *
		 * If no contexts are wanting the tooltip visible, it is hidden.
		 */
		TSet<FString> Contexts;

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
	};

	/**
	 * Resolves the player's widget state entry, creating it if not exists.
	 */
	FWidgetState& GetState(const APlayerController* Controller) const;

	/**
	 * Widget state data, tracked per player (indexed by player index).
	 */
	mutable TArray<FWidgetState> Entries;

	bool bIsEnabled;
};

namespace VulRuntime
{
	VULRUNTIME_API UVulTooltipSubsystem* Tooltip(const UObject* WorldCtx);
}