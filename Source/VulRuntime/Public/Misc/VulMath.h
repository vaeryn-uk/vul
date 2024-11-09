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
	 * Returns the two vectors that lie in a perpendicular line on Plane at point T along line segment A->B.
	 *
	 * Plane must be a plane's normal vector that is used to calculate the perpendicular.
	 *
	 * The two vectors returned will be Distance away from the line segment A->B at point T, where T
	 * is the point along the line (0=B, 1=A).
	 *
	 * This demonstrates the idea in 2D, where we return X & Y (T=0.5)
	 *
	 *                     A
	 *                     ^
	 *                     |
	 *      X <-Distance-> | <-Distance-> Y
	 *                     |
	 *                     |
	 *                     B
	 */
	static TArray<FVector> EitherSideOfLine(
		const FVector& A,
		const FVector& B,
		const float T,
		const FVector& Plane,
		const float Distance
	);

	/**
	 * How much must we turn from Start to End in 2D (ignoring any Z-locations).
	 *
	 * Returns a heading angle in radians, as per UE::Math::TVector::HeadingAngle.
	 */
	static float HeadingAngleBetween2D(const FTransform& Start, const FVector& End);

	/**
	 * Returns a point within a box indicated by Position.
	 *
	 * Position is a value between 0-1 for each axis, e.g. 0.5, 0.5, 0.5 will return
	 * the center of the box.
	 */
	static FVector PointInBox(const FBox& Box, const FVector& Position);
};
