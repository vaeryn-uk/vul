#include "Time/VulTime.h"

#include "VulRuntime.h"

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

FVulTime FVulTime::RealTime(UWorld* World)
{
	return FVulTime([World]
	{
		checkf(::IsValid(World), TEXT("Cannot generate now time as world is invalid"))
		return World->GetRealTimeSeconds();
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

float FVulTime::LoopedAlpha(const float TotalSeconds, const float Offset) const
{
	return fmod(Alpha(TotalSeconds) + Offset, 1.f);
}

float FVulTime::ClampedAlpha(const float TotalSeconds) const
{
	return FMath::Clamp(Alpha(TotalSeconds), 0, 1);
}

bool FVulTime::IsAfter(const float Seconds) const
{
	if (!IsValid())
	{
		return false;
	}

	return NowFn() > Time + Seconds;
}

bool FVulTime::IsNowOrAfter(const float Seconds) const
{
	if (!IsValid())
	{
		return false;
	}

	return NowFn() >= Time + Seconds;
}

float FVulTime::Seconds() const
{
	return Time;
}

float FVulTime::SecondsNow() const
{
	return NowFn();
}

void FVulTime::SetNow()
{
	Time = NowFn();
}

FVulFutureTime FVulFutureTime::WorldTime(UWorld* World, float SecondsInFuture)
{
	FVulFutureTime Ret;

	Ret.Time = FVulTime::WorldTime(World);
	Ret.Seconds = SecondsInFuture;

	return Ret;
}

bool FVulFutureTime::IsNowOrInPast() const
{
	return Time.IsNowOrAfter(Seconds);
}

bool FVulFutureTime::IsNowWithin(float const Before, float const After) const
{
	return FMath::IsWithin(Time.SecondsNow(), Time.Seconds() + Seconds - Before, Time.Seconds() + Seconds + After);
}

float FVulFutureTime::ClampedAlpha() const
{
	return Time.ClampedAlpha(Seconds);
}

FVulTimeWindow FVulTimeWindow::WorldTime(UWorld* World, const float Begin, const float Finish)
{
	FVulTimeWindow Ret;
	
	Ret.NowFn = [World]
	{
		checkf(::IsValid(World), TEXT("Cannot generate now time as world is invalid"))
		return World->GetTimeSeconds();
	};
	
	Ret.Start = Ret.NowFn() + Begin;
	Ret.End = Ret.NowFn() + Finish;
	
	return Ret;
}

float FVulTimeWindow::Alpha() const
{
	return (NowFn() - Start) / (End - Start);
}

float FVulTimeWindow::NowInWindow() const
{
	return FMath::IsWithin(Alpha(), 0, 1);
}

bool FVulTimeWindow::HasBegun() const
{
	return Alpha() >= 0.f;
}

bool FVulTimeWindow::HasFinished() const
{
	return Alpha() >= 1.f;
}
