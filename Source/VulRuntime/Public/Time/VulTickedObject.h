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
	 * Implement this to do your custom tick logic.
	 */
	virtual void VulTick() PURE_VIRTUAL(, return;);

	/**
	 * The rate at which VulTick should be called, in seconds.
	 */
	virtual float VulTickTime() PURE_VIRTUAL(, return 0.f; )
private:
	void SetLastTickTime();

	double TickedTime;

	double LastVulTickTime;
};
