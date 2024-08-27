#pragma once

#include "CoreMinimal.h"
#include "Components/Border.h"
#include "Tooltip/VulTooltipSubsystem.h"
#include "Time/VulTime.h"
#include "VulAnimatedHighlight.generated.h"

USTRUCT()
struct VULRUNTIME_API FVulAnimatedHighlightSettings
{
	GENERATED_BODY()

	/**
	 * After constructing settings, use functions to configure the settings, e.g. Background and Brightness.
	 */
	FVulAnimatedHighlightSettings() = default;
	FVulAnimatedHighlightSettings(const FMargin InPadding) : Padding(InPadding) {}

	/**
	 * Configures a background animation. Returns this for chaining.
	 */
	FVulAnimatedHighlightSettings Background(
		const FLinearColor& Default,
		const FLinearColor& Highlighted
	);

	/**
	 * Configures a brightness animation. Returns this for chaining.
	 */
	FVulAnimatedHighlightSettings Brightness(
		const float Default = .6f,
		const float Highlighted = 1.f
	);

	/**
	 * The brightness of content when not highlighted.
	 */
	UPROPERTY(EditAnywhere)
	float DefaultBrightness = 1.f;

	/**
	 * The brightness of content when highlighted.
	 */
	UPROPERTY(EditAnywhere)
	float HighlightedBrightness = 1.f;

	UPROPERTY(EditAnywhere)
	FLinearColor DefaultBackgroundColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere)
	FLinearColor HighlightedBackgroundColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere)
	FMargin Padding = FMargin(0);

	/**
	 * The animation speed between default & highlighted states.
	 */
	UPROPERTY(EditAnywhere)
	float Speed = DefaultSpeed;

	/**
	 * The default animation speed.
	 */
	static constexpr float DefaultSpeed = .15f;

	bool Animates() const;

	bool AnimatesBrightness() const;

	bool AnimatesBackground() const;
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

	void SetBackgroundColor(const FLinearColor& Color);
	void SetContentBrightness(const float Brightness);
};

template <typename T>
T* UVulAnimatedHighlight::GetTypedContent() const
{
	const auto Typed = Cast<T>(GetContent());

	checkf(IsValid(Typed), TEXT("UVulAnimatedHighlight does not contain content of the requested type"))

	return Typed;
}
