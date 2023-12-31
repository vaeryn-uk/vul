#pragma once

#include "Addr.h"

namespace VulRuntime::Hexgrid
{
	static FVector ProjectionPlane = FVector(1, 1, 0);

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
	VULRUNTIME_API FTransform CalculateMeshTransformation(const FBox& HexMeshBoundingBox, const float HexSize);

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
	VULRUNTIME_API FVector Project(const FVulHexAddr& Addr, const float HexSize);
}
