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

FVulHexShape FVulHexShape::Translate(const FVulHexVector& Vector) const
{
	TArray<FVulHexAddr> Out;

	for (const auto Tile : Tiles)
	{
		Out.Add(Tile.Translate(Vector));
	}

	return Out;
}

TOptional<FVulHexShape> FVulHexShape::RotateUntil(const TFunction<bool(const FVulHexShape&)> Filter) const
{
	for (int I = 0; I < 6; ++I)
	{
		if (const auto Rotated = Rotate(I); Filter(Rotated))
		{
			return Rotated;
		}
	}

	return {};
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

TArray<FVulHexAddr> FVulHexShape::GetTiles() const
{
	return Tiles;
}
