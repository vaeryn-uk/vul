#pragma once
#include "Misc/VulMath.h"

#include "VulHexAddr.generated.h"

/**
 * An integer wrapper that represents a rotation on our hexgrid.
 *
 * This is a value between 0-5, where 0 faces along positively along the Q axis,
 * +1 turns right, and -1 turns left.
 *
 * Default value is 0.
 */
USTRUCT()
struct VULRUNTIME_API FVulHexRotation
{
	GENERATED_BODY()

	FVulHexRotation() = default;
	FVulHexRotation(int InValue) : Value(FVulMath::Modulo(InValue, 6)) {}

	FVulHexRotation operator+(const FVulHexRotation Other) const;

	/**
	 * Returns the internal value. Guaranteed a value between 0-5.
	 */
	int GetValue() const;

private:
	uint8 Value = 0;
};

/**
 * Describes a translation between two tiles in a hexgrid.
 */
typedef UE::Math::TIntVector2<int> FVulHexVector;

/**
 * The address of a single tile in a 2D hexgrid.
 *
 * Uses a cube coordinate system (q, r, s).
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

	static FVulHexAddr Origin();

	/**
	 * From a vector, which is simply QR coords.
	 */
	FVulHexAddr(const FVulHexVector& V) : Q(V[0]), R(V[1]), S(-V[1] - V[0])
	{
		EnsureValid();
	}

	UPROPERTY(EditAnywhere)
	int Q = 0;
	UPROPERTY(EditAnywhere)
	int R = 0;
	UPROPERTY(EditAnywhere)
	int S = 0;

	FString ToString() const;

	/**
	 * Returns the translation to move this to Other.
	 */
	FVulHexVector Diff(const FVulHexAddr& Other) const;

	/**
	 * Returns the hex vector representation of this address.
	 *
	 * This is the QR components.
	 */
	FVulHexVector Vector() const;

	/**
	 * All the addresses that are adjacent to this address on a hexgrid.
	 *
	 * Note that the addresses returned may not be valid for a given grid due to its boundaries.
	 */
	TArray<FVulHexAddr> Adjacent() const;

	/**
	 * Rotates this address as presented on a hexgrid around the origin tile.
	 */
	FVulHexAddr Rotate(const FVulHexRotation& Rotation) const;

	/**
	 * Returns the address after moving this tile across the grid by QR (as per our q,r,s coord system).
	 */
	FVulHexAddr Translate(const FVulHexVector& Vector) const;

	/**
	 * Returns the closest hex rotation towards the Other address.
	 */
	FVulHexRotation RotationTowards(const FVulHexAddr& Other) const;

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

	bool IsValid() const;

	/**
	 * Returns the addresses that make up a hexagonal-shaped grid of the given size.
	 *
	 * Returns tiles expanding as rings around the origin hex. Size is how many rings
	 * there are.
	 */
	static TArray<FVulHexAddr> GenerateGrid(const int Size);

private:
	/**
	 * Generates the numeric sequence used to build rings around an origin tile.
	 */
	static TArray<int> GenerateSequenceForRing(const int Ring);

	void EnsureValid() const;
};

// For using as a key in maps.
FORCEINLINE uint32 GetTypeHash(const FVulHexAddr& Addr)
{
	return FCrc::MemCrc32(&Addr, sizeof(FVulHexAddr));
}