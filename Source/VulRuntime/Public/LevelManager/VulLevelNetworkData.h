#pragma once

#include "CoreMinimal.h"
#include "VulLevelSpawnActor.h"
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

	UPROPERTY(VisibleAnywhere)
	FString RequestId;

	UPROPERTY(VisibleAnywhere)
	FName LevelName;

	UPROPERTY(VisibleAnywhere)
	double IssuedAt;

	UPROPERTY(VisibleAnywhere)
	double CompletedAt = -1;

	/**
	 * How many clients have completed loading?
	 *
	 * This excludes the server.
	 */
	UPROPERTY(VisibleAnywhere)
	int32 ClientsLoaded = 0;
	
	/**
	 * How many clients must have completed loading before we actually proceed?
	 *
	 * This excludes the server.
	 */
	UPROPERTY(VisibleAnywhere)
	int32 ClientsTotal = 0;

	/**
	 * Is the server itself ready to switch?
	 */
	UPROPERTY(VisibleAnywhere)
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

	/**
	 * For debugging in editor.
	 */
	UPROPERTY(Replicated, VisibleAnywhere)
	FString LevelManagerId;

	UPROPERTY(Replicated, VisibleAnywhere)
	bool IsServer = false;
	
	UPROPERTY(ReplicatedUsing=OnRep_StateChange, VisibleAnywhere)
	FName CurrentLevel;

	/**
	 * How a server informs clients of its load progress.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_StateChange, VisibleAnywhere)
	FVulPendingLevelRequest PendingPrimaryLevelRequest;

	/**
	 * How clients update the client as to their load progress.
	 */
	UPROPERTY(VisibleAnywhere)
	FVulPendingLevelRequest PendingClientLevelRequest;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostNetInit() override;

	/**
	 * For the server, stores level actors that have been spawned.
	 *
	 * Clients can inspect this to find their replicated copies of an actor.
	 */
	UPROPERTY(Replicated)
	TArray<FVulLevelManagerSpawnedActor> ServerSpawnedClientActors = {};

	/**
	 * Actors that are spawned by & for the server, but also replicated to clients.
	 *
	 * These are made available for convenience so a client can retrieve these via
	 * LevelManagedActor.
	 */
	UPROPERTY(Replicated)
	TArray<FVulLevelManagerSpawnedActor> ServerSpawnedActors = {};

	void SetPendingClientLevelRequest(const FVulPendingLevelRequest& New);
	void SetPendingClientLevelManagerId(const FString& Id);

private:
	/**
	 * RPC for a client to share its own Request status to the server.
	 */
	UFUNCTION(Server, Reliable)
	void Server_UpdateClientRequest(const FVulPendingLevelRequest& Request);
	UFUNCTION(Server, Reliable)
	void Server_UpdatePendingLevelManagerId(const FString& Id);
	
	UFUNCTION()
	void OnRep_StateChange();
};
