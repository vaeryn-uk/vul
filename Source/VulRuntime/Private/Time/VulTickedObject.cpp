#include "Time/VulTickedObject.h"

bool UVulTickedObject::IsAllowedToTick() const
{
	return !HasAnyFlags(RF_ClassDefaultObject);
}

void UVulTickedObject::Tick(float DeltaTime)
{
	if (!IsVulTickingPaused())
	{
		TickedTime += DeltaTime;

		if (TickedTime - LastVulTickTime > VulTickTime())
		{
			// TODO: Multiple ticks?
			VulTick();
			SetLastTickTime();
		}
	}
}

TStatId UVulTickedObject::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVulTickedObject, STATGROUP_Tickables);
}

double UVulTickedObject::VulTimeSpentTicking() const
{
	return TickedTime;
}

void UVulTickedObject::SetLastTickTime()
{
	// Normalize last tick to the last Xms to ensure that we don't lose time
	// when actual game engine ticks fall after this Xms. E.g., without this
	// normalization, assuming X=100:
	//    tick 1 = 102ms, next>=202ms
	//    tick 2 = 205ms, next>=305ms
	//    tick 3 = 308ms, next>=408ms
	//    tick 4 = 412ms, next>=412ms.
	// ...Continues to get more out of sync.
	//
	// Instead, with normalization, the next is always set to 200, 300, 400ms,
	// etc. so we tick as soon as we should.
	LastVulTickTime = FMath::RoundToFloat(TickedTime / VulTickTime()) * VulTickTime();
}
