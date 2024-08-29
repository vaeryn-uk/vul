#include "UserInterface/VulAnimatedHighlight.h"

FVulAnimatedHighlightSettings FVulAnimatedHighlightSettings::Background(
	const FLinearColor& Default,
	const FLinearColor& Highlighted
) {
	DefaultBackgroundColor = Default;
	HighlightedBackgroundColor = Highlighted;

	return *this;
}

FVulAnimatedHighlightSettings FVulAnimatedHighlightSettings::Brightness(const float Default, const float Highlighted)
{
	DefaultBrightness = Default;
	HighlightedBrightness = Highlighted;

	return *this;
}

bool FVulAnimatedHighlightSettings::Animates() const
{
	return AnimatesBackground() || AnimatesBrightness();
}

bool FVulAnimatedHighlightSettings::AnimatesBrightness() const
{
	return DefaultBrightness != HighlightedBrightness;
}

bool FVulAnimatedHighlightSettings::AnimatesBackground() const
{
	return DefaultBackgroundColor != HighlightedBackgroundColor;
}

UVulAnimatedHighlight* UVulAnimatedHighlight::Wrap(
	const FVulAnimatedHighlightSettings& InSettings,
	UWidget* ToWrap
) {
	// Create this in the same way as WidgetTree::ConstructWidget.
	// Feels a bit strange to have the content be the owner of the thing that's
	// wrapping it, but this saves the caller having to provide a blueprint widget tree
	// or similar. Seems to work like this, and a simpler API.
	const auto Created = NewObject<UVulAnimatedHighlight>(ToWrap, StaticClass(), NAME_None, RF_Transactional);

	if (!ensureAlwaysMsgf(IsValid(Created), TEXT("Failed to spawn UVulAnimatedHighlight wrapper")))
	{
		return Created;
	}

	Created->SetPadding(InSettings.Padding);
	Created->SetContent(ToWrap);
	Created->SetContentBrightness(InSettings.DefaultBrightness);
	Created->SetBackgroundColor(InSettings.DefaultBackgroundColor);
	Created->Settings = InSettings;

	return Created;
}

void UVulAnimatedHighlight::Tick(float DeltaTime)
{
	if (!IsValid(this) || !IsValid(GetWorld()) || !ChangedAt.IsSet() || !Settings.Animates())
	{
		return;
	}

	const float Alpha = ChangedAt->Alpha(Settings.Speed);

	if (Settings.AnimatesBrightness())
	{
		if (Alpha < 1) {
			const auto Start = bIsHighlighted ? Settings.DefaultBrightness : Settings.HighlightedBrightness;
			const auto End   = bIsHighlighted ? Settings.HighlightedBrightness : Settings.DefaultBrightness;
			SetContentBrightness(FMath::Lerp(Start, End, Alpha));
		} else
		{
			SetContentBrightness(bIsHighlighted ? Settings.HighlightedBrightness : Settings.DefaultBrightness);
		}
	}

	if (Settings.AnimatesBackground())
	{
		if (Alpha < 1) {
			const auto Start = bIsHighlighted ? Settings.DefaultBackgroundColor : Settings.HighlightedBackgroundColor;
			const auto End   = bIsHighlighted ? Settings.HighlightedBackgroundColor : Settings.DefaultBackgroundColor;
			SetBackgroundColor(FMath::Lerp(Start, End, Alpha));
		} else
		{
			SetBackgroundColor(bIsHighlighted ? Settings.HighlightedBackgroundColor : Settings.DefaultBackgroundColor);
		}
	}
}

TStatId UVulAnimatedHighlight::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVulAnimatedHighlight, STATGROUP_Tickables);
}

bool UVulAnimatedHighlight::IsAllowedToTick() const
{
	return !HasAnyFlags(RF_ClassDefaultObject);
}

FVulTooltipWidgetOptions UVulAnimatedHighlight::TooltipOptions() const
{
	FVulTooltipWidgetOptions Options;

	Options.OnShow = [this] { const_cast<UVulAnimatedHighlight*>(this)->Activate(); };
	Options.OnHide = [this] { const_cast<UVulAnimatedHighlight*>(this)->Deactivate(); };

	return Options;
}

void UVulAnimatedHighlight::Activate()
{
	ChangedAt = FVulTime::RealTime(GetWorld());
	bIsHighlighted = true;
}

void UVulAnimatedHighlight::Deactivate()
{
	ChangedAt = FVulTime::RealTime(GetWorld());
	bIsHighlighted = false;
}

void UVulAnimatedHighlight::SetBackgroundColor(const FLinearColor& Color)
{
	SetBrushColor(Color);
}

void UVulAnimatedHighlight::SetContentBrightness(const float Brightness)
{
	SetContentColorAndOpacity(FLinearColor(Brightness, Brightness, Brightness, GetContentColorAndOpacity().A));
}
