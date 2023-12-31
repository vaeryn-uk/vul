#include "Hexgrid/Util.h"
#include "Hexgrid/Addr.h"

FTransform VulRuntime::Hexgrid::CalculateMeshTransformation(const FBox& HexMeshBoundingBox, const float HexSize)
{
	const auto Size = HexMeshBoundingBox.GetSize();
	FTransform Out;

	FVector Plane;
	FVector Unscaled;

	if (Size.GetMin() == Size.X)
	{
		Plane = FVector(0, 1, 1);
		Unscaled = FVector(1, 0, 0);
	} else if (Size.GetMin() == Size.Y)
	{
		Plane = FVector(1, 0, 1);
		Unscaled = FVector(0, 1, 0);
	} else
	{
		Plane = FVector(1, 1, 0);
		Unscaled = FVector(0, 0, 1);
	}

	// Scale the mesh by the longest dimension (x2 length of a single hex side)
	auto Scale = Plane * (HexSize / Size.GetMax()) * 2;
	// Preserve the flat plane of the mesh.
	Out.SetScale3D(Scale + Unscaled);

	return Out;
}

FVector VulRuntime::Hexgrid::Project(const FVulHexAddr& Addr, const float HexSize)
{
	const auto XUnit = FMath::Sqrt(FMath::Square(HexSize) - FMath::Square(HexSize / 2));
	const auto YUnit = HexSize * 1.5;

	return FVector(
		2 * XUnit * Addr.Q + (XUnit * Addr.R),
		(YUnit * -Addr.R),
		0
	);
}
