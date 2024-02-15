#pragma once

#include "CoreMinimal.h"
#include "VulHexAddr.h"
#include "UObject/Object.h"
#include "VulHexShape.generated.h"

/**
 * A shape in a hexagonal grid; a collection of directions that "walk along" the shape.
 *
 * Supports only contiguous shapes (cannot represents two independent shapes that are not connected
 * by at least two adjacent tiles).
 *
 * This is conceptual representation disconnected from any specific hexgrid. Internally, this
 * stores the information needed to draw the shape and provides functionality to project
 * this on to a grid.
 */
USTRUCT()
struct VULRUNTIME_API FVulHexVectorShape
{
	GENERATED_BODY()

	FVulHexVectorShape() = default;
	FVulHexVectorShape(const TArray<FVulHexRotation>& InDirections) : Directions(InDirections) {};

	/**
	 * Projects this shape on to a hexgrid, returning the tiles that make up the shape.
	 */
	TArray<FVulHexAddr> Project(
		const FVulHexAddr& Origin       = FVulHexAddr(),
		const FVulHexRotation& Rotation = FVulHexRotation()) const;

private:
	/**
	 * The turns we take whilst walking along this shape.
	 */
	TArray<FVulHexRotation> Directions;
};
