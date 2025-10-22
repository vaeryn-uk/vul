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
	 * If replicated, these will be 
	 */
	Server,

	/**
	 * One actor is spawned per connected client. Each is created on the server
	 * and owned by its respective client, allowing Client -> Server RPCs.
	 *
	 * A dedicated server does not spawn an additional server-only copy.
	 */
	Client,

	/**
	 * An actor is spawned only for clients, and spawned locally. The server is unaware
	 * of these.
	 */
	ClientLocal,
};

USTRUCT(BlueprintType)
struct VULRUNTIME_API FVulLevelSpawnActorParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Actor;

	UPROPERTY(EditAnywhere)
	EVulLevelSpawnActorNetOwnership Network = EVulLevelSpawnActorNetOwnership::Local;

	bool ShouldSpawnOnClient() const;

	bool ShouldSpawnOnServer() const;
};

/**
 * These are attached to a level actor on the server to share information with the client.
 */
UCLASS()
class UVulLevelActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVulLevelActorComponent();

	UPROPERTY(Replicated)
	FName OwningLevelManager;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
