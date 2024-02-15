#include "Hexgrid/VulHexShape.h"

TArray<FVulHexAddr> FVulHexVectorShape::Project(const FVulHexAddr& Origin, const FVulHexRotation& Rotation) const
{
	TArray Out = {Origin};

	for (const auto Direction : Directions)
	{
		Out.Add(Out.Last().Adjacent(Rotation + Direction));
	}

	return Out;
}
