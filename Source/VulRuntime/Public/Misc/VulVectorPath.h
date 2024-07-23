#pragma once

#include "CoreMinimal.h"
#include "Time/VulTime.h"
#include "UObject/Object.h"
#include "VulVectorPath.generated.h"

/**
 * Configuration provided to the algorithm which applies curves to a vector path.
 */
struct VULRUNTIME_API FVulVectorPathCurveOptions
{
	/**
	 * A returned curved path is a series of points (so technically no curves are present).
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
	 */
	int Samples = 24;

	/**
	 * This algorithm walks the path, turning and sampling as we go. To find the end of the
	 * path, we check if we've come close to the last point in the path. TerminationFactor
	 * is multiplied by the whole path's distance to determine how close we must be to
	 * consider the path ended. Note that the final point of the curved path will always
	 * be the final point of this path.
	 */
	float TerminationFactor = 0.01;

	/**
	 * MaxLengthFactor is a control to ensure that we don't infinite loop. It is
	 * possible that given a limited turn value, or very low sampling, or a minuscule
	 * termination factor, the curved path we walk never actually meets its end criteria.
	 * As a failsafe, MaxLengthFactor is multiplied by this path's distance; if the curved
	 * path we're producing becomes longer than this max length, we bail and return an
	 * invalid path.
	 */
	float MaxLengthFactor = 3;

	/**
	 * Allows for pointing the initial path-walking in a certain direction. If omitted,
	 * the resulting path will start pointing directly at the second point along the path,
	 * thus no curve would be present until we reached the second point along the path.
	 */
	TOptional<FRotator> StartDirection = {};

	/**
	 * Optionally customize the rotation that is applied for each segment along the path.
	 *
	 * This accepts an adjustment to the path's direction this turn and expects a modified
	 * adjustment rotator that will then be applied to the current direction.
	 *
	 * This can be used to provided alternative rotations than the default/optimized one.
	 * For example, you could adjust a left/right (yaw) rotation to also go up/down (pitch)
	 * as it travels.
	 *
	 * RequiredTurn is additional information representing the complete rotation that is required
	 * to take the current direction of travel to the next point on the path. In other words,
	 * TurnThisSegment is a clamped version of RequiredTurn, as per the Curve function's TurnDegsPerWorldUnit
	 * input.
	 *
	 * Use with caution; if the returned rotation does not bring us closer the next path on
	 * the path, curve path calculation will fail.
	 */
	TFunction<FRotator(const FRotator& TurnThisSegment, const FRotator& RequiredTurn)> AdjustRotation = nullptr;
};

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
	 * By default, first and last are not randomized.
	 */
	FVulVectorPath Randomize(
		const FRandomStream& Stream,
		const FBox& Box,
		const bool First = false,
		const bool Last = false
	) const;

	/**
	 * Returns a new path which takes a curved route along this path.
	 *
	 * The curves taken are defined by TurnDegsPerWorldUnit. The route will head for the next
	 * point along the path, turning as much as is allowed to track towards it, when passing
	 * a point, we track towards the next one. Note this does not turn early ahead of corners,
	 * but will only initiate a turn after passing a point.
	 *
	 * See FVulVectorPathCurveOptions for more details on how this algorithm works and can be customized.
	 */
	FVulVectorPath Curve(
		const float TurnDegsPerWorldUnit,
		const FVulVectorPathCurveOptions& Options = FVulVectorPathCurveOptions()
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
	 * Converts the provided alpha value along the entire path to an alpha
	 * value between the last-but-one and last point.
	 *
	 * Returns -1 if the provided alpha value does not represent a point
	 * along the path between last-but-one and last point. Returns 1 if we're
	 * at the end (i.e. provided alpha >= 1).
	 */
	float FinalDestinationAlpha(const float Alpha) const;

	/**
	 * The total distance covered when traversing the full path.
	 */
	float GetDistance() const;

	/**
	 * True if this path has at least 2 points.
	 */
	bool IsValid() const;

	FVulVectorPath Translate(const FVector& By) const;

	/**
	 * Returns a copy of this path with its final point moved to NewEnd.
	 */
	FVulVectorPath RelocateEnd(const FVector& NewEnd) const;

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
	 *
	 * InMovementCurve can be provided to vary the speed of travel as an object progresses
	 * along the path. The linear alpha value (between 0-1) will be mapped via this function
	 * and the returned result is used instead.
	 */
	FVulPathMovement(
		const FVulVectorPath& InPath,
		const FVulTime& Now,
		float InDuration,
		const TFunction<float (float)>& InMovementCurve = {}
	) : Path(InPath), Started(Now), Duration(InDuration), MovementCurve(InMovementCurve) {}

	/**
	 * Moves the provided transform to the correct place on this path for the current time.
	 *
	 * Faces the transform in the direction it's travelling.
	 *
	 * AdjustDirection can be used to alter the rotation of the transform as it moves along
	 * the path. This is called with the calculated rotator and current alpha value.
	 */
	FTransform Apply(
		const FTransform& Current,
		const TFunction<FRotator(const FRotator Calculated, const float Alpha)>& AdjustDirection = nullptr
	) const;

	/**
	 * Returns true if the movement is completed. Usually this means this movement object can be trashed.
	 */
	bool IsComplete() const;

	float GetDuration() const;

	const FVulVectorPath& GetPath() const;
private:
	FVulVectorPath Path;
	FVulTime Started;
	float Duration;
	TFunction<float (float)> MovementCurve;
};