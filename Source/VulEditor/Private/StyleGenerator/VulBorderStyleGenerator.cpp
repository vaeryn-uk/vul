#include "StyleGenerator/VulBorderStyleGenerator.h"
#include "StyleGenerator/VulStyleGeneration.h"
#include "UserInterface/VulMultiBorder.h"

void UVulBorderStyleGenerator::Generate()
{
	FVulStyleGeneration::GenerateStyles<UVulMultiBorderStyle, FVulBorderStyleVariation>(
		nullptr,
		this,
		TEXT("MultiBorderStyle"),
		Variations,
		[this](UVulMultiBorderStyle* Style, const FVulBorderStyleVariation& Variation)
		{
			Style->Brushes = Variation.Brushes;
			Style->Padding = Variation.Padding;
		},
		false
	);
}
