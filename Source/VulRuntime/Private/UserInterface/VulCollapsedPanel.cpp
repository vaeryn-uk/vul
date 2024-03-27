#include "UserInterface/VulCollapsedPanel.h"

TSharedRef<SWidget> UVulCollapsedPanel::RebuildWidget()
{
	if (IsValid(Trigger))
	{
		Trigger->OnClicked().AddWeakLambda(this, [this]
		{
			ToggleContent();
		});
	}

	ToggleContent(bStartOpen);

	// Use an invisible border to contain our content.
	auto Widget = SNew(SBorder);

	Widget->SetPadding(FMargin(0));
	Widget->SetBorderImage(nullptr);

	if (GetContent())
	{
		Widget->SetContent(GetContent()->TakeWidget());
	}

	return Widget;
}

void UVulCollapsedPanel::ToggleContent(const TOptional<bool>& Force)
{
	const auto Desired = Force.IsSet() ? Force.GetValue() : !bIsOpen;

	SetVisibility(Desired ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	bIsOpen = Desired;
}
