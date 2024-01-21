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

	bool IsValid() const;

	/**
	 * Checks if we are within Seconds after we last started.
	 *
	 * False if we haven't started or is not valid.
	 */
	bool IsWithin(float Seconds) const;

	/**
	 * Checks if we are after Seconds after we last started.
	 *
	 * False if we haven't started or is not valid.
	 */
	bool IsAfter(float Seconds) const;

	/**
	 * Set the time to now.
	 */
	void SetNow();

private:
	FVulNowFn NowFn;
	float Time = -1;
};