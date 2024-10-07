#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VulLevelAwareActor.generated.h"

UINTERFACE()
class UVulLevelAwareActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * Actors that implement this will be notified when VulLevelManager switches
 * to/from levels that contain this actor.
 */
class VULRUNTIME_API IVulLevelAwareActor
{
	GENERATED_BODY()

public:
	/**
	 * Called when the level that owns this actor is shown.
	 *
	 * This is analogous to BeginPlay, but is guaranteed to run after Vul level management
	 * has shown the actor's level.
	 */
	virtual void OnVulLevelShown();
};
