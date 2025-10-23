#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VulLevelNetworkData.generated.h"

/**
 * Representation of an in-progress, synchronized server/client level load.
 *
 * This is used by both server and client representations by UVulLevelNetworkData.
 */
USTRUCT(BlueprintType)
struct VULRUNTIME_API FVulPendingLevelRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FString RequestId;

	UPROPERTY()
	FName LevelName;

	UPROPERTY()
	double IssuedAt;

	UPROPERTY()
	double CompletedAt = -1;

	/**
	 * How many clients have completed loading?
	 *
	 * This excludes the server.
	 */
	UPROPERTY()
	int32 ClientsLoaded = 0;
	
	/**
	 * How many clients must have completed loading before we actually proceed?
	 *
	 * This excludes the server.
	 */
	UPROPERTY()
	int32 ClientsTotal = 0;

	/**
	 * Is the server itself ready to switch?
	 */
	UPROPERTY()
	bool ServerReady = false;

	bool IsValid() const { return !LevelName.IsNone() && !RequestId.IsEmpty(); }
	bool IsComplete() const { return CompletedAt > 0; }
	bool IsPending() const { return IsValid() && !IsComplete(); }
};

/**
 * Stores information relating to the state of a server or client's UVulLevelManager.
 *
 * One of these is placed in to the level for each instance:
 * - Server, so clients can see the current level and follow.
 * - Client(s), so the server can track where client load progress and synchronize level changes.
 */
UCLASS()
class VULRUNTIME_API AVulLevelNetworkData : public AActor
{
	GENERATED_BODY()

public:
	AVulLevelNetworkData();

	DECLARE_MULTICAST_DELEGATE_OneParam(FVulServerLevelChange, AVulLevelNetworkData*)

	FVulServerLevelChange OnNetworkLevelChange;

	UPROPERTY(Replicated)
	bool IsServer = false;
	
	UPROPERTY(ReplicatedUsing=OnRep_StateChange)
	FName CurrentLevel;

	UPROPERTY(ReplicatedUsing=OnRep_StateChange)
	FVulPendingLevelRequest PendingServerLevelRequest;

	UPROPERTY()
	FVulPendingLevelRequest PendingClientLevelRequest;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostNetInit() override;

	/**
	 * For the server, stores level actors that have been spawned.
	 *
	 * Clients can inspect this to find their replicated copies of an actor.
	 */
	UPROPERTY(Replicated)
	TArray<AActor*> ServerSpawnedClientActors = {};

	/**
	 * RPC for a client to share its own Request status to the server.
	 */
	UFUNCTION(Server, Reliable)
	void Server_UpdateClientRequest(const FVulPendingLevelRequest& Request);

private:
	UFUNCTION()
	void OnRep_StateChange();
};
