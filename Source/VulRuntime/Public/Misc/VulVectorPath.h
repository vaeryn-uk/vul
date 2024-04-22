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
	 * Returns the underlying points that make up this path.
	 */
	TArray<FVector> GetPoints() const;

	/**
	 * Returns a new path whose points are randomized within the given box around each point.
	 *
	 * First and last are not randomized.
	 */
	FVulVectorPath Randomize(const FRandomStream& Stream, const FBox& Box) const;

	/**
	 * Returns a new path which takes a curved route along this path.
	 *
	 * The curves taken are defined by TurnDegsPerWorldUnit. The route will head for the next
	 * point along the path, turning as much as is allowed to track towards it, when passing
	 * a point, we track towards the next one. Note this does not turn early ahead of corners,
	 * but will only initiate a turn after passing a point.
	 *
	 * The returned path is a still series of points (so technically no curves are present).
	 * Samples controls the maximum number of points we sample along the route we travel.
	 * If a curve isn't necessary (e.g. we're travelling in a straight line to the
	 * next point), we won't add these points. A higher Samples value will produce smoother
	 * curves, at the cost of calculation time.
	 *
	 * Samples is divided by the average segment length to produce the distance between
	 * each sample. For this reason, this produces better outputs when segments along
	 * the path are roughly equal distance. This algorithm is quite heavy, and can produce
	 * paths with a much higher number of points that this path. This is particularly
	 * true for higher sample counts along winding paths.
	 *
	 * This algorithm walks the path, turning and sampling as we go. To find the end of the
	 * path, we check if we've come close to the last point in the path. TerminationFactor
	 * is multiplied by the whole path's distance to determine how close we must be to
	 * consider the path ended. Note that the final point of the curved path will always
	 * be the final point of this path.
	 *
	 * MaxLengthFactor is a control to ensure that we don't infinite loop. It is
	 * possible that given a limited turn value, or very low sampling, or a minuscule
	 * termination factor, the curved path we walk never actually meets its end criteria.
	 * As a failsafe, MaxLengthFactor is multiplied by this path's distance; if the curved
	 * path we're producing becomes long that this max length, we bail and return an
	 * invalid path.
	 */
	FVulVectorPath Curve(
		const float TurnDegsPerWorldUnit,
		const int Samples = 24,
		const float TerminationFactor = 0.01,
		const float MaxLengthFactor = 3,
		const TOptional<FRotator>& StartDirection = {}
	) const;

	/**
	 * An overload that curves starting from the specified direction.
	 */
	FVulVectorPath Curve(
		const float TurnDegsPerWorldUnit,
		const FRotator& StartDirection,
		const int Samples = 24,
		const float TerminationFactor = 0.01,
		const float MaxLengthFactor = 3
	) const;

	/**
	 * Returns a new path with any redundant points removed, i.e. removes any points
	 * that don't impact the direction of travel because 3 consecutive points sit
	 * in a straight line.
	 */
	FVulVectorPath Simplify() const;

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
	FTransform Apply(const FTransform& Current) const;

	/**
	 * Returns true if the movement is completed. Usually this means this movement object can be trashed.
	 */
	bool IsComplete() const;
private:
	FVulVectorPath Path;
	FVulTime Started;
	float Duration;
};