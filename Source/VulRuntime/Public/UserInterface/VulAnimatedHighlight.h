#pragma once

#include "CoreMinimal.h"
#include "Components/Border.h"
#include "Tooltip/VulTooltipSubsystem.h"
#include "Time/VulTime.h"
#include "Blueprint/WidgetTree.h"
#include "VulAnimatedHighlight.generated.h"

USTRUCT()
struct FVulAnimatedHighlightSettings
{
	GENERATED_BODY()

	/**
	 * The brightness of content when not highlighted.
	 */
	UPROPERTY(EditAnywhere)
	float DefaultBrightness = .6f;

	/**
	 * The brightness of content when highlighted.
	 */
	UPROPERTY(EditAnywhere)
	float HighlightedBrightness = 1.f;

	/**
	 * The animation speed between default & highlighted states.
	 */
	UPROPERTY(EditAnywhere)
	float Speed = .15f;
};

/**
 * A border widget that makes for convenient wrapping of content to be highlighted.
 *
 * This is an alternative to UMG widget animations that can be easily defined via settings
 * from CPP code, removing the need to manually define animations in the editor for the simple
 * case of highlight on/off.
 *
 * TODO: Maybe we can expand this content further in the future to provide more controls
 *       for simple on/off animations, such as opacity or a visible border.
 *
 * Do not construct this manually; use Wrap.
 */
UCLASS()
class VULRUNTIME_API UVulAnimatedHighlight : public UBorder, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/**
	 * Creates a new highlight widget and wraps the provided widget ToWrap.
	 */
	static UVulAnimatedHighlight* Wrap(const FVulAnimatedHighlightSettings& InSettings, UWidget* ToWrap);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/**
	 * Commonly used with tooltips, this creates & configures options to activate/deactivate this
	 * widget when a tooltip is shown. Provide the resulting options to VulRuntime::Tooltipify.
	 */
	FVulTooltipWidgetOptions TooltipOptions() const;

	void Activate();
	void Deactivate();

	UPROPERTY(EditAnywhere)
	FVulAnimatedHighlightSettings Settings;

	template <typename T>
	T* GetTypedContent() const;

private:
	bool bIsHighlighted;
	TOptional<FVulTime> ChangedAt;

	void SetContentBrightness(const float Brightness);
};

template <typename T>
T* UVulAnimatedHighlight::GetTypedContent() const
{
	const auto Typed = Cast<T>(GetContent());

	checkf(IsValid(Typed), TEXT("UVulAnimatedHighlight does not contain content of the requested type"))

	return Typed;
}
