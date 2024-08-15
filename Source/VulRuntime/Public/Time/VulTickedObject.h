#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulTickedObject.generated.h"

/**
 * Gives an object extended tick functionality: tick at a configured rate and
 * allow that ticking to pause independently from Unreal's game pause.
 */
UCLASS(Abstract)
class VULRUNTIME_API UVulTickedObject : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool IsAllowedToTick() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/**
	 * Total number of real-time seconds that this object has ticked (not paused).
	 */
	double VulTimeSpentTicking() const;

protected:
	/**
	 * Returns true when you want VulTick to stop being called temporarily.
	 */
	virtual bool IsVulTickingPaused() { return false; };

	/**
	 * Implement this to perform your custom tick logic.
	 *
	 * This function is only called whilst IsVulTickingPaused returns true and at a rate defined by VulTickTime.
	 *
	 * This tick is normalized such that we attempt to ensure the correct number of calls are made
	 * according to VulTickTime: if the engine is ticking slower than VulTickTime, VulTick will
	 * be called multiple times per engine tick to account for this.
	 *
	 * For example, if VulTickTime returns 100ms, but the engine is running at 5fps (200ms), VulTick
	 * will be called twice per engine tick, roughly ensuring that VulTick is fired 10 times per second.
	 *
	 * In theory, this is also respects varying rates of VulTickTime, though this hasn't been tested yet.
	 */
	virtual void VulTick() PURE_VIRTUAL(, return;);

	/**
	 * The rate at which VulTick should be called, in seconds.
	 */
	virtual float VulTickTime() const PURE_VIRTUAL(, return 0.f; )

	/**
	 * An estimation of how far between two VulTicks we are, where 0 = just ticked and 1 = going to tick again now.
	 *
	 * This isn't accurate, but is useful for smoothing against VulTick rate.
	 */
	double VulTickFraction() const;
private:
	void SetLastTickTime();

	double TickedTime = 0.0;

	double LastVulTickTime = 0.0;
};
