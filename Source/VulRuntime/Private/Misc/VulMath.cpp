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
