#include "Hexgrid/VulHexShape.h"

FVulHexShape FVulHexShape::Rotate(const FVulHexRotation& Rotation) const
{
	TArray<FVulHexAddr> Out;

	for (const auto Tile : Tiles)
	{
		Out.Add(Tile.Rotate(Rotation));
	}

	return Out;
}

FString FVulHexShape::ToString() const
{
	TArray<FString> Out;

	for (const auto Tile : Tiles)
	{
		Out.Add(Tile.ToString());
	}

	return FString::Join(Out, TEXT(", "));
}
