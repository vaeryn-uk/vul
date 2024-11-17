#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulTime.generated.h"

/**
 * Records a point in time given with control of a now function. Wraps common logic
 * for checking where we are now with relation to some time in the past.
 *
 * This offers a synchronous API intended for object tick code that will periodically
 * check the status of the timer. For asynchronous uses, use UE's FTimerManager.
 */
USTRUCT()
struct VULRUNTIME_API FVulTime
{
	typedef TFunction<float ()> FVulNowFn;

	GENERATED_BODY()

	FVulTime() = default;
	FVulTime(FVulNowFn InNowFn);

	/**
	 * Creates a time based on the current world time (respecting pauses).
	 */
	static FVulTime WorldTime(UWorld* World);

	/**
	 * Creates a time based on the current real time (pause ignored).
	 */
	static FVulTime RealTime(UWorld* World);

	bool IsValid() const;

	/**
	 * Checks if we are within Seconds after we last started.
	 *
	 * False if we haven't started or is not valid.
	 */
	bool IsWithin(float Seconds) const;

	/**
	 * Returns an alpha value (0-1) representing how much of the interval between this time and the provided TotalSeconds.
	 *
	 *  0 - this time is now, so none of the interval has passed.
	 *  1 - it is currently this time + TotalSeconds exactly; all of the interval has passed.
	 * >1 - the interval has passed by this amount (e.g. 2 = twice the interval has passed).
	 */
	float Alpha(const float TotalSeconds) const;

	/**
	 * Like Alpha, but will start again from 0 when we exceed 1.
	 *
	 * Offset allows adjusting the start of the loop by some fixed amount, e.g.
	 * 0.5 will start at 0.5, up to 1, then from 0 to 0.5 in a over TotalSeconds.
	 */
	float LoopedAlpha(const float TotalSeconds, const float Offset = 0.f) const;

	/**
	 * Returns Alpha, clamped between 0-1.
	 */
	float ClampedAlpha(const float TotalSeconds) const;

	/**
	 * Checks if we are after Seconds after we last started.
	 *
	 * False if we haven't started or is not valid.
	 */
	bool IsAfter(const float Seconds) const;
	bool IsNowOrAfter(const float Seconds) const;

	float Seconds() const;

	/**
	 * Returns Now as seconds using this FVulTime's NowFn.
	 *
	 * Does not modify the stored time.
	 */
	float SecondsNow() const;

	/**
	 * Set the time to now.
	 */
	void SetNow();

private:
	FVulNowFn NowFn;
	float Time = -1;
};

/**
 * A wrapper around an FVulTime where we know the duration we want to check for ahead of time.
 *
 * E.g. "I want to know when 5s has passed, then do something"
 */
USTRUCT()
struct VULRUNTIME_API FVulFutureTime
{
	GENERATED_BODY()

	static FVulFutureTime WorldTime(UWorld* World, float SecondsInFuture);

	/**
	 * Returns true if now has advanced past the stored time in the future.
	 */
	bool IsNowOrInPast() const;

	/**
	 * True if FutureTime-Before <= Now < FutureTime+After. Checks if the current time falls
	 * either side of a time.
	 */
	bool IsNowWithin(float const Before, float const After) const;

	/**
	 * Returns how close we are to the future time, based on when this time was set.
	 *
	 * 0 - we have just set this time.
	 * 1 - we have just reached the future time.
	 *
	 * Result is clamped between 0-1.
	 */
	float ClampedAlpha() const;

	FVulTime GetTime() const;
private:
	float Seconds = 0;
	FVulTime Time;
};

/**
 * A window of time, generally in the future.
 *
 * E.g. "after 3 seconds have passed, I want something to happen for 2 seconds".
 */
USTRUCT()
struct VULRUNTIME_API FVulTimeWindow
{
	GENERATED_BODY()

	/**
	 * Creates a new time window based on the world time. Begin and Finish are seconds in the future.
	 */
	static FVulTimeWindow WorldTime(
		UWorld* World,
		const float Begin,
		const float Finish
	);

	/**
	 * Returns now in relation to the time window:
	 *
	 * <0 if we've not yet begun.
	 * 0-1 if we're somewhere in the window.
	 * >1 if we're past the window.
	 * 
	 * Adjustment optionally allows us to check a different time.
	 * This is added to the time now before the check is performed.
	 */
	float Alpha(const float Adjustment = 0.f) const;

	/**
	 * True if we are currently in the middle the window (have begun, but not started).
	 */
	float NowInWindow() const;

	/**
	 * True if we have passed the start of the window (including if we've gone passed the window entirely).
	 */
	bool HasBegun() const;

	/**
	 * True if the window has completed.
	 */
	bool HasFinished() const;

private:
	float Start = -1;
	float End = -1;
	FVulTime::FVulNowFn NowFn;
};