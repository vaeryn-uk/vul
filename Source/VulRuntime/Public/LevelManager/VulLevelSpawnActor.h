#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VulLevelSpawnActor.generated.h"

/**
 * Under which conditions are actors spawned, based on their role in networked games.
 *
 * We use LevelManager's distinctions:
 * - primary: a game instance that is authoritative and running the full game logic.
 *            This may be an Unreal server, or a standalone game that's running single
 *            player.
 * - follower: a game instance that is following a primary; a client.
 */
UENUM(BlueprintType)
enum class EVulLevelSpawnActorNetOwnership : uint8
{
	/**
	 * Spawn an actor with no consideration for network or ownership.
	 *
	 * The actor is spawned independently on primary and followers, without replication or network ownership.
	 */
	Independent,
	
	/**
	 * Only spawn an actor on the primary instance.
	 *
	 * Use standard replication flags to decide if these are visible to any clients too.
	 * 
	 * If replicated, these will be available on followers via UVulLevelManager::GetLevelActor.
	 */
	Primary,

	/**
	 * One actor is spawned per connected player. Each is created on the primary
	 * and owned by its respective follower. This allows Unreal's Client -> Server RPCs.
	 *
	 * Followers will receive their copy of this actor, which can be retrieved via
	 * ULevelManager::GetLevelActor. A follower's level load will not be completed
	 * until its copy of this actor is available.
	 */
	PerPlayer,

	/**
	 * An actor is spawned only for followers or a client-based primary, and spawned locally.
	 */
	PlayerLocal,
};

UENUM(BlueprintType)
enum class EVulLevelSpawnActorPolicy : uint8
{
	/**
	 * This spawn will only last for the current level and always be destroyed when a later level is loaded.
	 *
	 * This is the default behavior.
	 */
	SpawnLevel,

	/**
	 * Actor is spawned in to the root level, unless an existing root level actor of the same class
	 * already exists, in which case the existing one is preserved.
	 */
	SpawnRoot_Preserve,

	/**
	 * Actor is spawned in to the root level. Unlike SpawnRoot_Preserve, this guarantees a fresh
	 * instance of the actor is spawned, replacing any that exist from a previous level
	 */
	SpawnRoot_New,
};

/**
 * Wraps a spawned actor instance with its persistence setting.
 */
USTRUCT(BlueprintType)
struct VULRUNTIME_API FVulLevelManagerSpawnedActor
{
	GENERATED_BODY()

	UPROPERTY()
	EVulLevelSpawnActorPolicy SpawnPolicy;
	
	UPROPERTY()
	AActor* Actor = nullptr;

	bool IsValid() const;

	bool operator==(const FVulLevelManagerSpawnedActor& Other) const;
};

USTRUCT(BlueprintType)
struct VULRUNTIME_API FVulLevelSpawnActorParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Actor;

	UPROPERTY(EditAnywhere)
	EVulLevelSpawnActorNetOwnership Network = EVulLevelSpawnActorNetOwnership::Independent;
	
	UPROPERTY(EditAnywhere)
	EVulLevelSpawnActorPolicy SpawnPolicy = EVulLevelSpawnActorPolicy::SpawnLevel;
};
