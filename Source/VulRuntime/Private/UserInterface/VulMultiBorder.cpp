#include "UserInterface/VulMultiBorder.h"
#include "Components/BorderSlot.h"

TSharedRef<SWidget> UVulMultiBorder::RebuildWidget()
{
	TArray<TSharedRef<SBorder>> Borders;
	FMargin ContentPadding;

	if (!Style.IsNull())
	{
		LoadedStyle = Style.LoadSynchronous()->GetDefaultObject<UVulMultiBorderStyle>();

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

FSlateBrush UVulMultiBorder::DefaultBrush = FSlateBrush();
