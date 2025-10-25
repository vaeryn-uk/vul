#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VulLevelSpawnActor.generated.h"

UENUM(BlueprintType)
enum class EVulLevelSpawnActorNetOwnership : uint8
{
	/**
	 * Spawn an actor with no consideration for network or ownership.
	 *
	 * The actor is spawned independently on the server and/or clients as desired,
	 * without replication or network ownership.
	 */
	Local,
	
	/**
	 * Only spawn an actor on the server.
	 *
	 * Use standard replication flags to decide if these are visible to any clients too.
	 * 
	 * If replicated, these will be available on a client via UVulLevelManager::GetLevelActor - TODO: implement.
	 */
	Server,

	/**
	 * One actor is spawned per connected client. Each is created on the server
	 * and owned by its respective client, allowing Client -> Server RPCs.
	 *
	 * A dedicated server does not spawn an additional server-only copy.
	 *
	 * Clients will receive their copy of this actor, which can be retrieved via
	 * ULevelManager::GetLevelActor. A client-side level load will not be completed
	 * until these actors are available.
	 */
	Client,

	/**
	 * An actor is spawned only for clients, and spawned locally. The server is unaware
	 * of these.
	 */
	ClientLocal,
};

UENUM(BlueprintType)
enum class EVulLevelSpawnActorPolicy : uint8
{
	/**
	 * This spawn will only last for the current level and always be destroyed when a subsequent level is loaded.
	 *
	 * This is the default behaviour.
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
	EVulLevelSpawnActorNetOwnership Network = EVulLevelSpawnActorNetOwnership::Local;
	
	UPROPERTY(EditAnywhere)
	EVulLevelSpawnActorPolicy SpawnPolicy = EVulLevelSpawnActorPolicy::SpawnLevel;

	bool ShouldSpawnOnClient() const;

	bool ShouldSpawnOnServer() const;
};
