#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VulHexgridSpawn.generated.h"

/**
 * A simple actor that indicates where a hexgrid should spawn.
 *
 * Placed in the world, but not visible.
 *
 * Note that the Vul library does not provide any hexgrid visualization actors directly,
 * as these will vary greatly depending on the game. This actor exists merely to assist
 * in those project actors' placement.
 */
UCLASS()
class VULRUNTIME_API AVulHexgridSpawn : public AActor
{
	GENERATED_BODY()

public:
	AVulHexgridSpawn();

	UPROPERTY()
	USceneComponent* Root = nullptr;

	/**
	 * Convenience to get the spawn location for a hexgrid.
	 *
	 * Assumes only one spawn actor in the world.
	 */
	static FVector GetSpawnLocation(UWorld* World);
};
