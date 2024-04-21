#pragma once

#include "CoreMinimal.h"
#include "Time/VulTime.h"
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
	 * Gets the next point along the path given the position indicated by Alpha (same as is used in Interpolate).
	 *
	 * If Alpha is >=1, this returns the last point of the path.
	 */
	FVector NextPoint(const float Alpha) const;

	/**
	 * Returns the rotation that an object would be at for the given alpha value.
	 */
	FRotator Direction(const float Alpha) const;

	/**
	 * The total distance covered when traversing the full path.
	 */
	float GetDistance() const;

	/**
	 * True if this path has at least 2 points.
	 */
	bool IsValid() const;

private:
	/**
	 * For the given alpha, returns the index of the last point on the path passed through.
	 */
	int LastPointIndex(const float Alpha) const;

	TArray<FVector> Points;

	/**
	 * Cached total distance.
	 */
	float Distance;

	void CalculateDistance();
};

/**
 * Models movement along a vector path over the provided duration.
 *
 * Can be used by Unreal actors to reposition themselves every tick to have them move along the provided path.
 */
struct VULRUNTIME_API FVulPathMovement
{
	FVulPathMovement() = default;

	/**
	 * Constructs a new path movement which starts immediately.
	 */
	FVulPathMovement(
		const FVulVectorPath& InPath,
		const FVulTime& Now,
		float InDuration
	) : Path(InPath), Started(Now), Duration(InDuration) {}

	/**
	 * Moves the provided transform to the correct place on this path for the current time.
	 *
	 * Faces the transform in the direction it's travelling.
	 */
	FTransform Apply(const FTransform& Current);

	/**
	 * Moves the transform along the path with a limit on the ability to turn by some amount.
	 *
	 * RotationLimit is the maximum number of degrees the rotation will be adjusted towards
	 * the next point on the path. This results in a curved travel path, rather than following the
	 * path exactly (assuming the RotationLimit is insufficient to turn on-path directly.
	 *
	 * When called in a Tick function, RotationLimit will often be calculated by:
	 *
	 *    RotationDegreesPerSecond * DeltaSeconds
	 *
	 * Internally this tracks when we last applied and each call is built off the last to
	 * produce a smooth movement.
	 */
	FTransform Apply(const FTransform& Current, const float RotationLimit);

	/**
	 * Returns true if the movement is completed. Usually this means this movement object can be trashed.
	 */
	bool IsComplete() const;
private:
	FVulVectorPath Path;
	FVulTime Started;
	float Duration;
	float LastAppliedAlpha = 0;
};