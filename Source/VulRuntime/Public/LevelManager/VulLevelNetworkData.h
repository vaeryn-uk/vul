#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VulLevelNetworkData.generated.h"

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
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentLevel)
	FName CurrentLevel;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(VisibleAnywhere)
	TMap<APlayerState*, FName> LastLoaded;
	
	UFUNCTION()
	void OnRep_CurrentLevel();
};
