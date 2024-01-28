#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulVectorPath.generated.h"

/**
 * Models a path in the world that represents an actor's path of travel.
 */
USTRUCT()
struct VULRUNTIME_API FVulVectorPath
{
	GENERATED_BODY()

	FVulVectorPath() = default;

	/**
	 * InPoints must be at least two points for this structure to be valid.
	 */
	FVulVectorPath(const TArray<FVector>& InPoints) : Points(InPoints)
	{
		CalculateDistance();
	}

	/**
	 * Returns the position along this path, where Alpha is a value between 0 (the start) and 1 (the end).
	 *
	 * Alpha will be clamped.
	 */
	FVector Interpolate(const float Alpha) const;

	/**
	 * The total distance covered when traversing the full path.
	 */
	float GetDistance() const;

	/**
	 * True if this path has at least 2 points.
	 */
	bool IsValid() const;

private:
	TArray<FVector> Points;

	/**
	 * Cached total distance.
	 */
	float Distance;

	void CalculateDistance();
};
