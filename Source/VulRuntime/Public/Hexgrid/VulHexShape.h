#pragma once

#include "CoreMinimal.h"
#include "VulHexAddr.h"
#include "UObject/Object.h"
#include "VulHexShape.generated.h"

/**
 * A shape in a hexagonal grid; a collection of tiles that make up a shape that can be
 * translated and rotated as one.
 */
USTRUCT()
struct VULRUNTIME_API FVulHexShape
{
	GENERATED_BODY()

	FVulHexShape() = default;

	/**
	 * Construct from the tiles that make up a shape.
	 */
	FVulHexShape(const TArray<FVulHexAddr>& InTiles) : Tiles(InTiles) {};

	/**
	 * Rotates this shape around the origin.
	 */
	FVulHexShape Rotate(const FVulHexRotation& Rotation) const;

	FString ToString() const;

private:
	/**
	 * The tiles that make up the shape on a hexgrid.
	 */
	TArray<FVulHexAddr> Tiles;
};
