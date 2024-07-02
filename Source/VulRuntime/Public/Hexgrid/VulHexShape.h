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

	/**
	 * Translates this shape by the given QR values.
	 */
	FVulHexShape Translate(const FVulHexVector& Vector) const;

	/**
	 * Starting with this shape, applies it to the given filter until the filter returns true.
	 *
	 * We rotate the shape each time, returning the first shape resulting from those rotation(s)
	 * where Filter returns true. The first rotation we try is the 0 rotation (i.e. this unrotated).
	 *
	 * Returns unset if Filter does not return true for any of the 6 rotations.
	 */
	TOptional<FVulHexShape> RotateUntil(const TFunction<bool (const FVulHexShape&)> Filter) const;

	FString ToString() const;

	TArray<FVulHexAddr> GetTiles() const;

private:
	/**
	 * The tiles that make up the shape on a hexgrid.
	 */
	TArray<FVulHexAddr> Tiles;
};
