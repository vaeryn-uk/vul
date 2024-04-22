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
