#include "UserInterface/VulAnimatedHighlight.h"
#include "VulRuntime.h"

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

	Created->SetPadding(FMargin(0));
	Created->SetContent(ToWrap);
	Created->Background.TintColor = FColor::Transparent;
	Created->SetContentBrightness(InSettings.DefaultBrightness);
	Created->Settings = InSettings;

	return Created;
}

void UVulAnimatedHighlight::Tick(float DeltaTime)
{
	if (!ChangedAt.IsSet())
	{
		return;
	}

	const float Alpha = ChangedAt->Alpha(Settings.Speed);

	if (Alpha < 1)
	{
		const auto Start = bIsHighlighted ? Settings.DefaultBrightness : Settings.HighlightedBrightness;
		const auto End   = bIsHighlighted ? Settings.HighlightedBrightness : Settings.DefaultBrightness;
		SetContentBrightness(FMath::Lerp(Start, End, Alpha));
	} else
	{
		SetContentBrightness(bIsHighlighted ? Settings.HighlightedBrightness : Settings.DefaultBrightness);
	}
}

TStatId UVulAnimatedHighlight::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVulAnimatedHighlight, STATGROUP_Tickables);
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

void UVulAnimatedHighlight::SetContentBrightness(const float Brightness)
{
	UE_LOG(LogVul, Display, TEXT("Setting brightness=%.2f"), Brightness);
	SetContentColorAndOpacity(FLinearColor(Brightness, Brightness, Brightness, GetContentColorAndOpacity().A));
}
