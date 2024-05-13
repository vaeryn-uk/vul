#include "StyleGenerator/VulButtonStyleGenerator.h"
#include "StyleGenerator/VulButtonStyle.h"
#include "StyleGenerator/VulStyleGeneration.h"

void UVulButtonStyleGenerator::Generate()
{
	FVulStyleGeneration::GenerateStyles<UVulButtonStyle, FVulButtonStyleVariation>(
		Template,
		this,
		TEXT("ButtonStyle"),
		Variations,
		[this](UVulButtonStyle* Style, const FVulButtonStyleVariation& Variation)
		{
			Style->NormalBase.SetResourceObject(Variation.NormalBackground);
			Style->NormalHovered.SetResourceObject(Variation.HoveredBackground);
			Style->NormalPressed.SetResourceObject(Variation.PressedBackground);
			Style->Disabled.SetResourceObject(Variation.DisabledBackground);
		}
	);
}
