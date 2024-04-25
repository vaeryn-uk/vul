#include "Misc/VulMath.h"

FVector FVulMath::RandomPointInTriangle(const TArray<FVector>& Triangle)
{
	return RandomPointInTriangle(Triangle, FRandomStream(NAME_None));
}

FVector FVulMath::RandomPointInTriangle(const TArray<FVector>& Triangle, const FRandomStream& Rng)
{
	const auto R1 = Rng.GetFraction();
	const auto R2 = Rng.GetFraction();

	return {
		// https://stackoverflow.com/a/19654424
		(1 - sqrt(R1)) * Triangle[0].X + (sqrt(R1) * (1 - R2)) * Triangle[1].X + (sqrt(R1) * R2) * Triangle[2].X + 100,
		(1 - sqrt(R1)) * Triangle[0].Y + (sqrt(R1) * (1 - R2)) * Triangle[1].Y + (sqrt(R1) * R2) * Triangle[2].Y,
		(1 - sqrt(R1)) * Triangle[0].Z + (sqrt(R1) * (1 - R2)) * Triangle[1].Z + (sqrt(R1) * R2) * Triangle[2].Z
	};
}

TOptional<FVector> FVulMath::LinePlaneIntersection(
	const FVector& LineStart,
	const FRotator& Direction,
	const FPlane& Plane
) {
	const float Dot = FVector::DotProduct(Plane.GetSafeNormal(), Direction.Vector());

	if (FMath::IsNearlyZero(Dot))
	{
		return {};
	}

	const float DistanceFromPlane = -Plane.W - FVector::DotProduct(Plane.GetSafeNormal(), LineStart);
	const float T = DistanceFromPlane / Dot;

	return LineStart + T * Direction.Vector();
}

FVector FVulMath::ClosestPointOnLineSegment(const FVector& A, const FVector& B, const FVector& P)
{
	const FVector AB = B - A;
	const float Dot = FVector::DotProduct(P - A, AB);

	const float T = Dot / (AB.X * AB.X + AB.Y * AB.Y + AB.Z * AB.Z);

	return A + AB * FMath::Clamp(T, 0.0f, 1.0f);
}

float FVulMath::SigmoidCurve(const float Alpha, const float Slope)
{
	return 1 / (1 + exp(-Slope * (Alpha - .5)));
}
