#pragma once

#include "CoreMinimal.h"
#include "VulLevelData.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "Time/VulTime.h"
#include "VulLevelManager.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FVulLevelDelegate, const UVulLevelData*, const class UVulLevelManager*)

/**
 * Configuration for a level manager, see AVulLevelManager below.
 *
 * These settings are extracted so they can be configured in project settings as well
 * as on an actor directly.
 */
USTRUCT()
struct FVulLevelSettings
{
	GENERATED_BODY()

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
	 *
	 * Note that this maybe overridden in UVulRuntimeSettings::StartLevelOverride.
	 */
	UPROPERTY(EditAnywhere)
	FName StartingLevelName = NAME_None;

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

	TOptional<TPair<FName, UVulLevelData*>> FindLevel(UWorld* World) const;

	bool IsValid() const;
};

/**
 * Responsible for loading levels using Unreal's streaming level model.
 *
 * This provides a simple framework for switching levels with a loading screen
 * in between. This provides hooks for all stages of the loading process, and
 * can be configured & customized with your own definitions of level data which is
 * passed to all the relevant hooks.
 *
 * This subsystem will be auto-activated if UVulRuntimeSettings::LevelSettings is configured.
 *
 * Actors may implement IVulLevelAwareActor to be notified when levels are toggled
 * by this manager.
 */
UCLASS()
class VULRUNTIME_API UVulLevelManager : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual bool IsAllowedToTick() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	FVulLevelDelegate OnLevelLoadComplete;

	UPROPERTY(EditAnywhere)
	FVulLevelSettings Settings;

	/**
	 * Load a level by its name, invoking OnComplete when it is finished.
	 *
	 * If LevelName is already loaded, this will force a reload, destroying the
	 * existing level and streaming it in again fresh.
	 */
	void LoadLevel(const FName& LevelName, FVulLevelDelegate::FDelegate OnComplete = FVulLevelDelegate::FDelegate());

	/**
	 * Loads a level by an enum value.
	 *
	 * This allows for a stronger binding of CPP code to well-known levels. Instead of passing in magic names,
	 * you can define a UENUM of your game's levels, then pass in these enum values.
	 *
	 * The level manager is still expected to have its level data configured correctly in the editor.
	 *
	 * For example,
	 *
	 *   UENUM()
	 *   class enum EMyProjectLevels : uint8
	 *   {
	 *       LevelOne,
	 *       LevelTwo,
	 *   }
	 *
	 * In level manager actor, you'll need to define level data for "LevelOne" and "LevelTwo" to use these.
	 *
	 * TODO: Could simply offer a EnumToName function in Vul to easily reuse this concept across
	 * other use-cases, rather than needing to embed it in vul level manager directly.
	 */
	template <typename EnumType>
	void LoadLevel(const EnumType Level, FVulLevelDelegate::FDelegate OnComplete = FVulLevelDelegate::FDelegate())
	{
		FName LevelName = FName(StaticEnum<EnumType>()->GetNameStringByValue(static_cast<int64>(Level)));
		LoadLevel(LevelName, OnComplete);
	}

	/**
	 * Returns parameters for spawning in an actor that belongs to the currently-loaded level.
	 */
	FActorSpawnParameters SpawnParams();

	/**
	 * Gets a widget spawned as a result of the last level load of the given type.
	 *
	 * Returns nullptr if a suitable widget cannot be found.
	 */
	template <typename WidgetType>
	WidgetType* LastSpawnedWidget() const;

	/**
	 * Returns the level last-loaded by this level manager, or nullptr.
	 */
	ULevelStreaming* GetLastLoadedLevel() const;

private:
	UVulLevelData* ResolveData(const FName& LevelName);

	ULevelStreaming* GetLevelStreaming(const FName& LevelName, const TCHAR* FailReason = TEXT(""));

	void ShowLevel(const FName& LevelName);
	void HideLevel(const FName& LevelName);

	/**
	 * Initializes the level manager with the provided settings in normal operation.
	 *
	 * Will immediately load the first level, as per settings.
	 */
	void InitLevelManager(const FVulLevelSettings& InSettings, UWorld* World);

	/**
	 * Generates a unique action info input required for streaming levels. Required for the level streaming API.
	 */
	FLatentActionInfo NextLatentAction();

	/**
	 * The last level that was successfully loaded by this manager (including loading level).
	 */
	TWeakObjectPtr<ULevelStreaming> LastLoadedLevel = nullptr;

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

	/**
	 * Caches the level data defined for each level.
	 */
	UPROPERTY()
	mutable TMap<FName, UVulLevelData*> LevelDataInstances;

	void LoadAssets(const TArray<FSoftObjectPath>& Paths);
	bool AreWaitingForAdditionalAssets() const;

	FStreamableManager StreamableManager;

	/**
	 * A handle for the additional assets the last level load request.
	 *
	 * This handle can be used to release assets.
	 */
	TSharedPtr<FStreamableHandle> AdditionalAssets;

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

	/**
	 * Tracks the widgets spawned when showing the last level.
	 */
	TArray<TWeakObjectPtr<UWidget>> Widgets;

	/**
	 * Tracks whether we still need to level-aware actors' on shown function.
	 *
	 * We do not immediately invoke these calls; instead we tick until the level
	 * is completely ready, then fire them.
	 */
	bool bIsPendingActorOnShow = false;

	/**
	 * True if this level manager is in its normal level streaming mode. This is set
	 * false when we're loading the game with a non-root level (i.e. from the editor with
	 * a specific level open). In this scenario, we call actor & widgets hooks for that
	 * level, but do not support level switching.
	 */
	bool bIsInStreamingMode = true;

	/**
	 * This tracks whether we need an OnLevelShow call. We need a player controller for this,
	 * so attempt it each tick until one is available. If this is set, we're awaiting a call
	 * against this data.
	 */
	TWeakObjectPtr<UVulLevelData> OnShowLevelData;

	FDelegateHandle WorldInitDelegateHandle;

	void CallActorsLevelShown(ULevel* Level);
};

template <typename WidgetType>
WidgetType* UVulLevelManager::LastSpawnedWidget() const
{
	for (const auto& Widget : Widgets)
	{
		if (!Widget.IsValid())
		{
			continue;
		}

		if (const auto Casted = Cast<WidgetType>(Widget.Get()); IsValid(Casted))
		{
			return Casted;
		}
	}

	return nullptr;
}

namespace VulRuntime
{
	VULRUNTIME_API UVulLevelManager* LevelManager(UWorld* WorldCtx);
}