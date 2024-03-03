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
};
