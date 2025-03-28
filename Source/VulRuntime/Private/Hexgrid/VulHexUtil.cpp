﻿#include "Hexgrid/VulHexUtil.h"
#include "Hexgrid/VulHexAddr.h"
#include "Kismet/KismetMathLibrary.h"

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

TArray<TArray<FVector>> VulRuntime::Hexgrid::Triangles(
	const FVulHexAddr& Addr,
	const FVulWorldHexGridSettings& GridSettings,
	const float Scale)
{
	const auto Center = Project(Addr, GridSettings);
	const auto Verts = Points(Addr, GridSettings, Scale);

	return {
		{Verts[5], Center, Verts[0]},
		{Verts[0], Center, Verts[1]},
		{Verts[1], Center, Verts[2]},
		{Verts[2], Center, Verts[3]},
		{Verts[3], Center, Verts[4]},
		{Verts[4], Center, Verts[5]},
	};
}

TArray<FVector> VulRuntime::Hexgrid::Points(
	const FVulHexAddr& Addr,
	const FVulWorldHexGridSettings& GridSettings,
	const float Scale,
	const bool IncludeCenter
) {
	const auto Center = Project(Addr, GridSettings);
	TArray<FVector> Out;

	if (IncludeCenter)
	{
		Out.Add(Center);
	}

	for (auto N = 0; N < 6; ++N)
	{
		Out.Add(FVector(
			Center.X + UKismetMathLibrary::DegCos(30 + 60 * N) * GridSettings.HexSize * Scale,
			Center.Y + UKismetMathLibrary::DegSin(30 + 60 * N) * GridSettings.HexSize * Scale,
			0
		));
	}

	return Out;
}

FVulHexAddr VulRuntime::Hexgrid::Deproject(
	const FVector& WorldLocation,
	const FVulWorldHexGridSettings& GridSettings,
	const FVector& GridOrigin
) {
	const auto WorldOffset = WorldLocation - GridOrigin;

	const int R = FMath::RoundToInt(WorldOffset.Y / GridSettings.LongStep() * - 1);
	const int Q = FMath::RoundToInt((WorldOffset.X - GridSettings.ShortStep() * R) / 2.0 / GridSettings.ShortStep());

	return FVulHexAddr(Q, R);
}

FVector VulRuntime::Hexgrid::RandomPointInTile(
	const FVulHexAddr& Addr,
	const FVulWorldHexGridSettings& GridSettings,
	const float Scale)
{
	return RandomPointInTile(Addr, GridSettings, FRandomStream(NAME_None), Scale);
}

FVector VulRuntime::Hexgrid::RandomPointInTile(
	const FVulHexAddr& Addr,
	const FVulWorldHexGridSettings& GridSettings,
	const FRandomStream& Rng,
	const float Scale)
{
	const auto Tris = Triangles(Addr, GridSettings, Scale);
	// Each triangle is equal size, so choose one at random so we get uniform randomness.
	return FVulMath::RandomPointInTriangle(Tris[Rng.RandHelper(6)]);
}

FVulVectorPath VulRuntime::Hexgrid::VectorPath(
	const FVulHexAddr& Start,
	const TArray<FVulHexAddr>& Path,
	const FVulWorldHexGridSettings& GridSettings,
	const FVector& GridOrigin
) {
	TArray<FVector> Points;
	TArray Tiles = {Start};
	Tiles.Append(Path);

	for (const auto Tile : Tiles)
	{
		Points.Add(Project(Tile, GridSettings) + GridOrigin);
	}

	return FVulVectorPath(Points);
}

FVulHexAddr VulRuntime::Hexgrid::AveragePosition(const TArray<FVulHexAddr>& Tiles)
{
	TMap<FVulHexAddr, float> WeightedTiles;

	for (const auto& Tile : Tiles)
	{
		WeightedTiles.Add(Tile, 1);
	}

	return AveragePosition(WeightedTiles);
}

FVulHexAddr VulRuntime::Hexgrid::AveragePosition(const TMap<FVulHexAddr, float>& WeightedTiles)
{
	if (WeightedTiles.IsEmpty())
	{
		return FVulHexAddr::Origin();
	}

	FVector2D QR{};
	float Total = 0;

	for (const auto& Entry : WeightedTiles)
	{
		QR.X += Entry.Key.Q * Entry.Value;
		QR.Y += Entry.Key.R * Entry.Value;
		Total += Entry.Value;
	}

	return FVulHexAddr(static_cast<int>(QR.X / Total), static_cast<int>(QR.Y / Total));
}

float FVulWorldHexGridSettings::ShortStep() const
{
	return FMath::Sqrt(FMath::Square(HexSize) - FMath::Square(HexSize / 2));
}

float FVulWorldHexGridSettings::LongStep() const
{
	return HexSize * 1.5;
}
