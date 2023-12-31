#pragma once

#include "Addr.h"
#include "Util.generated.h"

/**
 * Defines settings required for the following utility functions.
 */
USTRUCT(BlueprintType)
struct FVulWorldHexGridSettings
{
	GENERATED_BODY()

	FVulWorldHexGridSettings() = default;
	FVulWorldHexGridSettings(float const InHexSize) : HexSize(InHexSize) {};

	/**
	 * The size of one side of a hex in world units. This controls how large a grid will be in the world.
	 */
	UPROPERTY(EditAnywhere)
	float HexSize = 50.0f;

	/**
	 * The plane on which the grid is projected.
	 *
	 * TODO: This isn't respected yet. Functions assume this value does not change.
	 */
	UPROPERTY(VisibleAnywhere)
	FPlane ProjectionPlane = FPlane(FVector::Zero(), FVector::ZAxisVector);

	/**
	 * The center of the grid in world space.
	 */
	UPROPERTY(EditAnywhere)
	FVector Origin = FVector::Zero();

	/**
	 * Returns the value between two hexes center points when moving one hex in the short direction.
	 *
	 *                +     +
	 *
	 *      +     +             +    ▲
	 *                               | Short step
	 *  +             +     +        ▼
	 *
	 *      +     +
	 */
	float ShortStep() const;

	/**
	 * Returns the value between two hexes center points when moving one hex in the long direction.
	 *
	 *                +     +
	 *
	 *      +     +      X      +
	 *
	 *  +      X      +     +
	 *
	 *      +     +
	 *
	 *         ◀---------▶
	 *           LongStep
	 */
	float LongStep() const;
};

namespace VulRuntime::Hexgrid
{

	/**
	 * Given a mesh, returns a transformation to apply to that mesh to ensure that its sides
	 * of the provided length.
	 *
	 * This is designed to be used alongside @see FVulHexAddr::Project.
	 *
	 * TODO: This doesn't deal with rotation (yet). The provided mesh must be "flat" in the XY plane.
	 *
	 * Assumes the provided mesh contains a regular hexagon, where all sides of equal length.
	 */
	VULRUNTIME_API FTransform CalculateMeshTransformation(
		const FBox& HexMeshBoundingBox,
		const FVulWorldHexGridSettings& GridSettings);

	/**
	 * Returns the center of the position of a hex as applied on a grid starting at (0, 0, 0).
	 *
	 * This assumes a top-down view, so the returned vector extends in X and Y coordinates.
	 *
	 * HexSize is the length of one side of each hex and we assume all hexes are regular hexagons
	 * of equal size.
	 *
	 * Spacing can be set to provide a uniform spacing between all hexes in the grid.
	 *
	 * Any further transformation (offset, rotation etc), is left to the caller.
	 */
	VULRUNTIME_API FVector Project(const FVulHexAddr& Addr, const FVulWorldHexGridSettings& GridSettings);

	VULRUNTIME_API FVulHexAddr Deproject(const FVector& WorldLocation, const FVulWorldHexGridSettings& GridSettings);
}
