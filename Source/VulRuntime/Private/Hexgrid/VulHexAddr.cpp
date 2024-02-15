#include "Hexgrid/VulHexAddr.h"

FVulHexRotation FVulHexRotation::operator+(const FVulHexRotation Other) const
{
	return FVulMath::Modulo(Value + Other.Value, 6);
}

int FVulHexRotation::GetValue() const
{
	return FVulMath::Modulo(static_cast<int>(Value), 6);
}

FString FVulHexAddr::ToString() const
{
	return FString::Printf(TEXT("(%d %d %d)"), Q, R, S);
}

TArray<FVulHexAddr> FVulHexAddr::Adjacent() const
{
	return {
		FVulHexAddr(Q + 1, R),
		FVulHexAddr(Q, R + 1),
		FVulHexAddr(Q - 1, R + 1),
		FVulHexAddr(Q - 1, R),
		FVulHexAddr(Q, R - 1),
		FVulHexAddr(Q + 1, R - 1),
	};
}

FVulHexAddr FVulHexAddr::Rotate(const FVulHexRotation& Rotation) const
{
	/*
	 * Implementation derived from pattern matching on the coords of a rotated tile.
	 * Not certain this is mathematically sound.
	 * Example coords.
	 * 0: +2 +1 -3 (start coords)
	 * 1. -1 +3 -2 (rotate 1 hex-side to the right)
	 * 2. -3 +2 +1 (rotate 2 hex-sides to the right)
	 * 3. -2 -1 +3 (rotate 3 hex-sides to the right)
	 * 4. +1 -3 +2 (rotate 4 hex-sides to the right)
	 * 5. +3 -2 -1 (rotate 5 hex-sides to the right)
	 */
	switch (Rotation.GetValue())
	{
	case 1:
		return FVulHexAddr(-1 * R, -1 * S);
	case 2:
		return FVulHexAddr(S, Q);
	case 3:
		return FVulHexAddr(-1 * Q, -1 * R);
	case 4:
		return FVulHexAddr(R, S);
	case 5:
		return FVulHexAddr(-1 * S, -1 * Q);
	default:
		return *this;
	}
}

bool FVulHexAddr::AdjacentTo(const FVulHexAddr& Other) const
{
	return Adjacent().Contains(Other);
}

int FVulHexAddr::Distance(const FVulHexAddr& Other) const
{
	return (FMath::Abs(Other.Q - Q) + FMath::Abs(Other.R - R) + FMath::Abs(Other.S - S)) / 2;
}

TArray<int> FVulHexAddr::GenerateSequenceForRing(const int Ring)
{
	auto AtLimitFor = 0;
	auto Current = 0;
	auto Direction = -1;
	TArray<int> Out;

	do
	{
		auto New = FMath::Clamp(Current, -Ring, Ring);
		Out.Add(New);

		if (New == -Ring || New == Ring)
		{
			if (++AtLimitFor > Ring)
			{
				Direction *= -1;
				Current = New;
			}
		} else
		{
			AtLimitFor = 0;
		}

		Current += Direction;
	} while (Out.Num() < Ring * 6);

	return Out;
}

bool FVulHexAddr::IsValid() const
{
	return Q + R + S == 0;
}

void FVulHexAddr::EnsureValid() const
{
	checkf(IsValid(), TEXT("Hexgrid address %s is not valid"), *ToString())
}
