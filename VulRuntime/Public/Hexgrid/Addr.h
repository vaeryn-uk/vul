#pragma once

#include "Addr.generated.h"

/**
 * The address of a single tile in a 2D hexgrid.
 *
 * Uses a cube coordinate system.
 */
USTRUCT(BlueprintType)
struct VULRUNTIME_API FVulHexAddr
{
	GENERATED_BODY()

	FVulHexAddr() = default; // Required for some containers.

	FVulHexAddr(const int InQ, const int InR) : Q(InQ), R(InR), S(-InR - InQ)
	{
		EnsureValid();
	}

	UPROPERTY(EditAnywhere)
	int Q;
	UPROPERTY(EditAnywhere)
	int R;
	UPROPERTY(EditAnywhere)
	int S;

	FString ToString() const;

	/**
	 * All the addresses that are adjacent to this address on a hexgrid.
	 *
	 * Note that the addresses returned may not be valid for a given grid due to its boundaries.
	 */
	TArray<FVulHexAddr> Adjacent() const;

	/**
	 * True if this tile is adjacent to (a neighbor of) Other.
	 */
	bool AdjacentTo(const FVulHexAddr& Other) const;

	/**
	 * Returns the distance between this and another grid address.
	 *
	 * As the crow flies.
	 */
	int Distance(const FVulHexAddr& Other) const;

	bool operator==(const FVulHexAddr& Other) const
	{
		return Other.Q == Q && Other.R == R && Other.S == S;
	}

	static TArray<int> GenerateSequenceForRing(const int Ring);

	bool IsValid() const;

private:
	void EnsureValid() const;
};

// For using as a key in maps.
FORCEINLINE uint32 GetTypeHash(const FVulHexAddr& Addr)
{
	return FCrc::MemCrc32(&Addr, sizeof(FVulHexAddr));
}