#include "Misc/VulRngManager.h"

FRotator FVulRandomStream::RandomRotation(bool bYaw, bool bPitch, bool bRoll) const
{
	FRotator Out = FRotator::ZeroRotator;

	if (bYaw)
	{
		Out.Yaw = FRand() * 360.f;
	}

	if (bPitch)
	{
		Out.Pitch = FRand() * 360.f;
	}

	if (bRoll)
	{
		Out.Roll = FRand() * 360.f;
	}

	return Out;
}

FVector FVulRandomStream::RandPointOnBoxSurface(const FBox& Box) const
{
	// Randomly select one of the six faces
	const int Face = RandRange(0, 5);

	FVector RandomPoint;

	// Generate a random point on the selected face
	switch (Face)
	{
	case 0:     // negative x face
		RandomPoint = FVector(Box.Min.X, FRandRange(Box.Min.Y, Box.Max.Y), FRandRange(Box.Min.Z, Box.Max.Z));
		break;
	case 1:     // positive x face
		RandomPoint = FVector(Box.Max.X, FRandRange(Box.Min.Y, Box.Max.Y), FRandRange(Box.Min.Z, Box.Max.Z));
		break;
	case 2:     // negative y face
		RandomPoint = FVector(FRandRange(Box.Min.X, Box.Max.X), Box.Min.Y, FRandRange(Box.Min.Z, Box.Max.Z));
		break;
	case 3:     // positive y face
		RandomPoint = FVector(FRandRange(Box.Min.X, Box.Max.X), Box.Max.Y, FRandRange(Box.Min.Z, Box.Max.Z));
		break;
	case 4:     // negative z face
		RandomPoint = FVector(FRandRange(Box.Min.X, Box.Max.X), FRandRange(Box.Min.Y, Box.Max.Y), Box.Min.Z);
		break;
	default:     // positive z face
		RandomPoint = FVector(FRandRange(Box.Min.X, Box.Max.X), FRandRange(Box.Min.Y, Box.Max.Y), Box.Max.Z);
		break;
	}

	return RandomPoint;
}
