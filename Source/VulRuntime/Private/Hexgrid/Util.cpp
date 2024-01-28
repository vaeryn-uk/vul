#include "Hexgrid/Util.h"
#include "Hexgrid/Addr.h"

FTransform VulRuntime::Hexgrid::CalculateMeshTransformation(
	const FBox& HexMeshBoundingBox,
	const FVulWorldHexGridSettings& GridSettings)
{
	const auto Size = HexMeshBoundingBox.GetSize();
	FTransform Out;

	FVector MeshPlane;
	FVector Unscaled;

	if (Size.GetMin() == Size.X)
	{
		MeshPlane = FVector(0, 1, 1);
		Unscaled = FVector(1, 0, 0);
	} else if (Size.GetMin() == Size.Y)
	{
		MeshPlane = FVector(1, 0, 1);
		Unscaled = FVector(0, 1, 0);
	} else
	{
		MeshPlane = FVector(1, 1, 0);
		Unscaled = FVector(0, 0, 1);
	}

	// Scale the mesh by the longest dimension (x2 length of a single hex side)
	auto Scale = MeshPlane * (GridSettings.HexSize / Size.GetMax()) * 2;
	// Preserve the flat plane of the mesh.
	Out.SetScale3D(Scale + Unscaled);

	return Out;
}

FVector VulRuntime::Hexgrid::Project(const FVulHexAddr& Addr, const FVulWorldHexGridSettings& GridSettings)
{
	return FVector(
		2 * GridSettings.ShortStep() * Addr.Q + (GridSettings.ShortStep() * Addr.R),
		(GridSettings.LongStep() * -Addr.R),
		0
	);
}

FVulHexAddr VulRuntime::Hexgrid::Deproject(const FVector& WorldLocation, const FVulWorldHexGridSettings& GridSettings)
{
	const int R = FMath::RoundToInt(WorldLocation.Y / GridSettings.LongStep() * - 1);
	const int Q = FMath::RoundToInt((WorldLocation.X - GridSettings.ShortStep() * R) / 2.0 / GridSettings.ShortStep());

	return FVulHexAddr(Q, R);
}

FVulVectorPath VulRuntime::Hexgrid::VectorPath(
	const FVulHexAddr& Start,
	const TArray<FVulHexAddr>& Path,
	const FVulWorldHexGridSettings& GridSettings)
{
	TArray<FVector> Points;
	TArray Tiles = {Start};
	Tiles.Append(Path);

	for (const auto Tile : Tiles)
	{
		Points.Add(Project(Tile, GridSettings));
	}

	return FVulVectorPath(Points);
}

float FVulWorldHexGridSettings::ShortStep() const
{
	return FMath::Sqrt(FMath::Square(HexSize) - FMath::Square(HexSize / 2));
}

float FVulWorldHexGridSettings::LongStep() const
{
	return HexSize * 1.5;
}
