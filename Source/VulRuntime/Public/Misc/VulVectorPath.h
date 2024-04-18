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
	FVulPathMovement(const FVulVectorPath& InPath, const FVulTime& Now, float InDuration) : Path(InPath), Started(Now),
		Duration(InDuration)
	{
	}

	/**
	 * Moves the provided actor to the correct place on this path for the current time.
	 *
	 * Will not move the actor if the movement path is complete.
	 *
	 * Faces the actor in the direction it's travelling.
	 */
	void Apply(AActor* ToMove);

	/**
	 * Returns true if the movement is completed. Usually this means this movement object can be trashed.
	 */
	bool IsComplete() const;
private:
	FVulVectorPath Path;
	FVulTime Started;
	float Duration;
};