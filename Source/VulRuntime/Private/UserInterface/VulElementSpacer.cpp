﻿#include "UserInterface/VulElementSpacer.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UPanelSlot* FVulElementSpacer::AddToContainer(UPanelWidget* Container, UWidget* Element) const
{
	if (const auto HBox = Cast<UHorizontalBox>(Container); HBox)
	{
		return Cast<UPanelSlot>(AddToContainer(HBox, Element));
	}

	if (const auto VBox = Cast<UVerticalBox>(Container); VBox)
	{
		return Cast<UPanelSlot>(AddToContainer(VBox, Element));
	}

	checkf(false, TEXT("Attempt to apply FVulElementSpacer to a container that is not vbox or hbox"))

	return nullptr;
}

UHorizontalBoxSlot* FVulElementSpacer::AddToContainer(UHorizontalBox* Container, UWidget* Element) const
{
	const auto Slot = Container->AddChildToHorizontalBox(Element);

	Slot->SetPadding(FMargin(Spacing / 2, 0));

	return Slot;
}

UVerticalBoxSlot* FVulElementSpacer::AddToContainer(UVerticalBox* Container, UWidget* Element) const
{
	const auto Slot = Container->AddChildToVerticalBox(Element);

	Slot->SetPadding(FMargin(0, Spacing / 2));

	return Slot;
}
