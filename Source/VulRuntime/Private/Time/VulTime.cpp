#include "Time/VulTime.h"

FVulTime::FVulTime(FVulNowFn InNowFn)
{
	NowFn = InNowFn;
	SetNow();
}

FVulTime FVulTime::WorldTime(UWorld* World)
{
	return FVulTime([World]
	{
		checkf(::IsValid(World), TEXT("Cannot generate now time as world is invalid"))
		return World->GetTimeSeconds();
	});
}

bool FVulTime::IsValid() const
{
	return NowFn != nullptr && Time >= 0;
}

bool FVulTime::IsWithin(float Seconds) const
{
	if (!IsValid())
	{
		return false;
	}

	return NowFn() <= Time + Seconds;
}

float FVulTime::Alpha(const float TotalSeconds) const
{
	return (NowFn() - Time) / TotalSeconds;
}

bool FVulTime::IsAfter(const float Seconds) const
{
	if (!IsValid())
	{
		return false;
	}

	return NowFn() > Time + Seconds;
}

void FVulTime::SetNow()
{
	Time = NowFn();
}
