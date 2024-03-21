#include "UserInterface/VulMultiBorder.h"
#include "Components/BorderSlot.h"

TSharedRef<SWidget> UVulMultiBorder::RebuildWidget()
{
	TArray<TSharedRef<SBorder>> Borders;
	FMargin ContentPadding;

	if (!Style.IsNull())
	{
		const auto LoadedStyle = Style.LoadSynchronous()->GetDefaultObject<UVulMultiBorderStyle>();

		for (const auto& Brush : LoadedStyle->Brushes)
		{
			Borders.Add(CreateBorder(&Brush));
		}

		for (auto I = Borders.Num() - 2; I >= 0; I--)
		{
			Borders[I]->SetContent(Borders[I + 1]);
		}

		ContentPadding = LoadedStyle->Padding;
	}

	// No/invalid style.
	if (Borders.IsEmpty())
	{
		Borders.Add(CreateBorder(&DefaultBrush));
	}

	if (GetChildrenCount() > 0)
	{
		Borders.Last()->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
		Borders.Last()->SetPadding(ContentPadding);
	}

	return Borders[0];
}

TSharedRef<SBorder> UVulMultiBorder::CreateBorder(TAttribute<const FSlateBrush*> Brush)
{
	auto Ret = SNew(SBorder);

	Ret->SetPadding(0);
	Ret->SetBorderImage(Brush);

	return Ret;
}

void UVulMultiBorder::PostLoad()
{
	Super::PostLoad();

	// Initialize our slot to be a border slot. Copied from UBorder.
	if ( GetChildrenCount() > 0 )
	{
		if ( UPanelSlot* PanelSlot = GetContentSlot() )
		{
			UBorderSlot* BorderSlot = Cast<UBorderSlot>(PanelSlot);
			if ( BorderSlot == NULL )
			{
				BorderSlot = NewObject<UBorderSlot>(this);
				BorderSlot->Content = GetContentSlot()->Content;
				BorderSlot->Content->Slot = BorderSlot;
				Slots[0] = BorderSlot;
			}
		}
	}
}
