﻿#pragma once

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

private:
	float Seconds = 0;
	FVulTime Time;
};