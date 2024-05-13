#include "StyleGenerator/VulTextStyleGenerator.h"
#include "StyleGenerator/VulStyleGeneration.h"

void UVulTextStyleGenerator::Generate()
{
	FVulStyleGeneration::GenerateStyles<UVulTextStyle, FVulTextStyleVariation>(
		Template,
		this,
		TEXT("TextStyle"),
		Variations,
		[this](UVulTextStyle* Style, const FVulTextStyleVariation& Variation)
		{
			// Adjust for DPI scaling applied to fonts to ensure the created
			// style has the same value as is defined in the variation.
			Style->Font.Size = Variation.FontSize / (96.f / 72.f);
			Style->Font.OutlineSettings.OutlineSize = Variation.OutlineSize;

			if (Variation.bApplyColor)
			{
				Style->Color = Variation.Color;
			}
		}
	);
}
