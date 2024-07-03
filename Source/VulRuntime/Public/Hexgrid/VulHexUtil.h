#pragma once

#include "VulHexAddr.h"
#include "Misc/VulVectorPath.h"
#include "VulHexUtil.generated.h"

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
	 * Default lays the grid along the XY plane.
	 *
	 * TODO: This isn't fully respected yet. Most functions assume this value does not change.
	 */
	UPROPERTY(VisibleAnywhere)
	FPlane ProjectionPlane = FPlane(FVector(0, 0, 1), 0);

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
	 *                   |
	 *  +      X      +  |  +
	 *         |         |
	 *      +  |  +      |
	 *         |         |
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
		const FVulWorldHexGridSettings& GridSettings
	);

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
	 * Note that we project X in ShortStep and Y in long step.
	 *
	 * Any further transformation (offset, rotation etc), is left to the caller.
	 */
	VULRUNTIME_API FVector Project(const FVulHexAddr& Addr, const FVulWorldHexGridSettings& GridSettings);

	/**
	 * Returns the 6 equilateral triangles that make up a hex tile at the given Addr.
	 *
	 *                  0
	 *            X-----------X
	 *       5  /   \       /   \  1
	 *        /       \   /       \
	 *      X-----------------------X
	 *        \       /   \       /
	 *       4  \   /       \   /  2
	 *            X-----------X
	 *                  3
	 *
	 * Scale is as per Points.
	 */
	VULRUNTIME_API TArray<TArray<FVector>> Triangles(
		const FVulHexAddr& Addr,
		const FVulWorldHexGridSettings& GridSettings,
		const float Scale = 1
	);

	/**
	 * Like Project, maps a tile on to world space but returns the 6 corners of the hex.
	 *
	 *      5     0
	 *
	 *  4             1
	 *
	 *      3     2
	 *
	 * Scale allows for scaling of the size of the hex' points from its center, but note this only
	 * effects the tile we're getting points for. We do not scale other tiles/the grid as a whole.
	 */
	VULRUNTIME_API TArray<FVector> Points(
		const FVulHexAddr& Addr,
		const FVulWorldHexGridSettings& GridSettings,
		const float Scale = 1
	);

	/**
	 * Takes a world location and returns the hex grid address this point sits within, according to GridSettings.
	 *
	 * The inverse of Project.
	 */
	VULRUNTIME_API FVulHexAddr Deproject(const FVector& WorldLocation, const FVulWorldHexGridSettings& GridSettings);

	/**
	 * Calculates a random world point inside the Addr tile whilst using GridSettings.
	 *
	 * Scale can extend/restrict the size of a hex available to pick a point from, for example
	 * a scale of .8 will ensure that the points are picked from the inner 80% of the hex, and
	 * a 20% border at the edge of the tile is excluded.
	 */
	VULRUNTIME_API FVector RandomPointInTile(
		const FVulHexAddr& Addr,
		const FVulWorldHexGridSettings& GridSettings,
		const float Scale = 1
	);

	/**
	 * Calculates a random world point inside the Addr tile whilst using GridSettings.
	 *
	 * Rng can be provided for deterministic randomization.
	 */
	VULRUNTIME_API FVector RandomPointInTile(
		const FVulHexAddr& Addr,
		const FVulWorldHexGridSettings& GridSettings,
		const FRandomStream& Rng,
		const float Scale = 1
	);

	/**
	 * Converts the provided start position and path to a path made of positions in the world.
	 *
	 * This can be used in conjunction with the results from a Path query to visualize actors moving
	 * along a hexgrid path.
	 */
	VULRUNTIME_API FVulVectorPath VectorPath(
		const FVulHexAddr& Start,
		const TArray<FVulHexAddr>& Path,
		const FVulWorldHexGridSettings& GridSettings
	);
}
