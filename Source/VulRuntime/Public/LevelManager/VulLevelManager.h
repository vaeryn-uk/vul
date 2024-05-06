#pragma once

#include "CoreMinimal.h"
#include "VulLevelData.h"
#include "Engine/StreamableManager.h"
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

	/**
	 * Attempts to retrieve a level manager in the given world. This is a convenience
	 * function for easy global access to a level manager. Expects a level manager
	 * actor in the root world.
	 */
	static AVulLevelManager* Get(UWorld* World);

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

	FVulLevelDelegate OnLevelLoadComplete;

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

	virtual void Tick(float DeltaTime) override;

	/**
	 * Load a level by its name, invoking OnComplete when it is finished.
	 *
	 * If LevelName is already loaded, this will force a reload, destroying the
	 * existing level and streaming it in again fresh.
	 */
	void LoadLevel(const FName& LevelName, FVulLevelDelegate::FDelegate OnComplete = FVulLevelDelegate::FDelegate());

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UVulLevelData* ResolveData(const FName& LevelName);

	ULevelStreaming* GetLevelStreaming(const FName& LevelName, const TCHAR* FailReason = TEXT(""));

	void ShowLevel(const FName& LevelName);
	void HideLevel(const FName& LevelName);

	/**
	 * Generates a unique action info input required for streaming levels. Required for the level streaming API.
	 */
	FLatentActionInfo NextLatentAction();

	/**
	 * The current level being loaded or has loaded.
	 *
	 * This will only be explicitly loaded levels (not the loading screen).
	 */
	TOptional<FName> CurrentLevel = {};

	/**
	 * The previously loaded CurrentLevel. We use this to track its unload state.
	 *
	 * Only set whilst an unload is in progress.
	 */
	TOptional<FName> WaitForUnload = {};

	/**
	 * Maintains a unique value so our streamed load requests don't collide with one another.
	 */
	int32 LoadingUuid;

	UPROPERTY()
	TMap<FName, UVulLevelData*> LevelDataInstances;

	void LoadAssets(const TArray<FSoftObjectPath>& Paths);
	void OnAssetLoaded();

	TArray<FSoftObjectPath> AssetsToLoad;

	FStreamableManager StreamableManager;

	bool bIsLoadingAssets = false;

	void LoadStreamingLevel(const FName& Name, TSoftObjectPtr<UWorld> Level);
	void UnloadStreamingLevel(const FName& Name, TSoftObjectPtr<UWorld> Level);

	/**
	 * Removes all widgets from all contained levels' viewports.
	 */
	static void RemoveAllWidgets(UWorld* World);

	/**
	 * Each request is stored in a queue internally.
	 */
	struct FLoadRequest
	{
		/**
		 * If not set, a request is simply a request to unload the current level.
		 */
		TOptional<FName> LevelName;
		FVulLevelDelegate Delegate;
		TOptional<FVulTime> StartedAt;
		bool IsLoadingLevel;
	};

	FLoadRequest* CurrentRequest();
	TArray<FLoadRequest> Queue;

	void StartProcessing(FLoadRequest* Request);
	void Process(FLoadRequest* Request);
	void NextRequest();

	bool IsReloadOfSameLevel(const FName& LevelName) const;
};
