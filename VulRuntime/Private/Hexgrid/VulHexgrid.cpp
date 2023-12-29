#include "Hexgrid/VulHexgrid.h"

FString FVulHexAddr::ToString() const
{
	return FString::Printf(TEXT("(%d %d %d)"), Q, R, S);
}

TArray<FVulHexAddr> FVulHexAddr::Adjacent() const
{
	return {
		FVulHexAddr(Q, R - 1),
		FVulHexAddr(Q, R + 1),
		FVulHexAddr(Q + 1, R - 1),
		FVulHexAddr(Q - 1, R + 1),
		FVulHexAddr(Q + 1, R),
		FVulHexAddr(Q - 1, R),
	};
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
