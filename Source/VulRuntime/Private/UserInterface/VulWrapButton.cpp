#include "UserInterface/VulWrapButton.h"
#include "Blueprint/WidgetTree.h"

UVulWrapButton* UVulWrapButton::WrapWidget(UUserWidget* Owner, UWidget* ToWrap)
{
	const auto Wrapper = Owner->WidgetTree->ConstructWidget<UVulWrapButton>();
	Wrapper->SetContent(ToWrap);
	return Wrapper;
}

TSharedRef<SWidget> UVulWrapButton::RebuildWidget()
{
	SetBackgroundColor(FLinearColor::Transparent);
	OnClicked.AddUniqueDynamic(this, &UVulWrapButton::UVulWrapButton::TriggerWrapButtonClicked);

	return Super::RebuildWidget();
}

void UVulWrapButton::TriggerWrapButtonClicked()
{
	WrapButtonClicked.Broadcast(GetContent());
}
