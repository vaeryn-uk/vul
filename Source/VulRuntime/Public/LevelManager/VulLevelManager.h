#pragma once

#include "CoreMinimal.h"
#include "VulLevelData.h"
#include "GameFramework/Actor.h"
#include "Time/VulTime.h"
#include "VulLevelManager.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FVulLevelDelegate, const UVulLevelData*)

/**
 * Responsible for loading levels using Unreal's streaming level model.
 *
 * This provides a simple framework for switching levels with a loading screen
 * in between. This provides hooks for all stages of the loading process, and
 * can be configured & customized with your own definitions of level data which is
 * passed to all the relevant hooks.
 *
 * This actor should be placed in the root level, and will load all levels in and out
 * as sub-levels. The root level persists throughout the entire game's lifespan.
 */
UCLASS()
class VULRUNTIME_API AVulLevelManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AVulLevelManager();

	UPROPERTY(EditAnywhere)
	TMap<FName, TSubclassOf<UVulLevelData>> LevelData;

	/**
	 * The name of the level in the level data that has a special designation as the
	 * loading level. That is, one that is shown whilst our levels are loading in and out.
	 */
	UPROPERTY(EditAnywhere)
	FName LoadingLevelName = FName("Loading");

	/**
	 * The name of the level that is loaded when the game starts. This is loaded as soon as this
	 * actor starts, if provided.
	 */
	UPROPERTY(EditAnywhere)
	FName StartingLevelName = NAME_None;

	FVulLevelDelegate OnLevelLoadBegin;
	FVulLevelDelegate OnLevelLoadComplete;

	ULevelStreaming* GetLevelStreaming(const FName& LevelName, const TCHAR* FailReason = TEXT(""));

	/**
	 * If showing the load screen, this is the minimum amount of time it will be displayed.
	 *
	 * This prevents strange flickering for really fast level loads.
	 */
	UPROPERTY(EditAnywhere)
	FTimespan MinimumTimeOnLoadScreen = FTimespan::FromSeconds(1);

	/**
	 * Maximum amount of time we'll wait for a level to load before failing.
	 */
	UPROPERTY(EditAnywhere)
	FTimespan LoadTimeout = FTimespan::FromSeconds(10);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/**
	 * Load a level by its name.
	 */
	void LoadLevel(const FName& LevelName);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UVulLevelData* ResolveData(const FName& LevelName);

	void ShowLevel(const FName& LevelName);
	void HideLevel(const FName& LevelName);

	/**
	 * Generates a unique action info input required for streaming levels. Required for the level streaming API.
	 */
	FLatentActionInfo NextLatentAction();

	/**
	 * If set, indicates we're currently loading a level.
	 */
	TOptional<FVulTime> LevelLoadStartedAt;

	/**
	 * The current level being loaded or has loaded.
	 */
	TOptional<FName> CurrentLevel;

	/**
	 * The previously loaded level. We use this to track its unload state.
	 *
	 * Only set whilst loading.
	 */
	TOptional<FName> PreviousLevel;

	/**
	 * Maintains a unique value so our streamed load requests don't collide with one another.
	 */
	int32 LoadingUuid;

	UPROPERTY()
	TMap<FName, UVulLevelData*> LevelDataInstances;
};
