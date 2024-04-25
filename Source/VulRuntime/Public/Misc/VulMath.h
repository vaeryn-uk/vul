#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * Generic math functions.
 */
class VULRUNTIME_API FVulMath
{
public:
	/**
	 * Modulo function returns the remainder after division, always returning a positive value.
	 *
	 * https://stackoverflow.com/a/1082938
	 */
	template <typename NumberType>
	static NumberType Modulo(const NumberType Index, const NumberType Divisor)
	{
		return (Index % Divisor + Divisor) % Divisor;
	}

	/**
	 * Gets a random point in a triangle with a non-deterministic Rng.
	 */
	static FVector RandomPointInTriangle(const TArray<FVector>& Triangle);

	/**
	 * Gets a random point in a triangle with deterministic Rng.
	 */
	static FVector RandomPointInTriangle(const TArray<FVector>& Triangle, const FRandomStream& Rng);

	/**
	 * A variant of UKismetMathLibrary::LinePlaneIntersection that takes a line start & direction, instead
	 * of a line segment.
	 *
	 * This calculates at which point, if any, the provided line will intersect plane. Returns unset if
	 * no intersection occurs.
	 */
	static TOptional<FVector> LinePlaneIntersection(const FVector& LineStart, const FRotator& Direction, const FPlane& Plane);

	/**
	 * Returns the point along line segment AB that is closest to P.
	 */
	static FVector ClosestPointOnLineSegment(const FVector& A, const FVector& B, const FVector& P);

	/**
	 * Plots a sigmoid curve, returning a Y value in range 0-1 from the curve for the given X.
	 *
	 * https://en.m.wikipedia.org/wiki/Logistic_function
	 *
	 * When applied as a speed curve, accelerates slowly, reaching a max speed in the middle,
	 * smoothly decelerates. 0.5 is the midpoint.
	 *
	 * Slope controls how steep the curve is, thus how slowly we ramp up & down.
	 * Note that with more gradual curves/a lower slope, 0 and 1 values will not necessarily
	 * remap to 0 and 1 exactly. E.g.
	 *
	 * - for slope=4: x=0, y=0.119 and x=1, y=0.881
	 * - for slope=16: x=0,y=0.0003 and x=1,y=0.9996
	 *
	 * The default is chosen for how close these values are to 0 and 1 inputs.
	 */
	static float SigmoidCurve(const float X, const float Slope = 16);
};
