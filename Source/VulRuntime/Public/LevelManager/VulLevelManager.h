#pragma once

#include "CoreMinimal.h"
#include "VulLevelData.h"
#include "VulLevelNetworkData.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"
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

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UWorld> RootLevel;

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
	 * Optionally specifies a different level for dedicated servers to start on.
	 *
	 * Note clients acting as a server do not count, only headless servers respect this.
	 */
	UPROPERTY(EditAnywhere)
	FName ServerStartingLevelName = NAME_None;

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

	FString Summary(const bool IsDedicatedServer) const;
	
	FName GetStartingLevelName(const bool IsDedicatedServer) const;
};

/**
 * Data made available when a level is shown.
 */
USTRUCT()
struct VULRUNTIME_API FVulLevelShownInfo
{
	GENERATED_BODY()

	/**
	 * The level manager instance that is currently managing levels.
	 */
	UPROPERTY()
	class UVulLevelManager* LevelManager;

	/**
	 * The world the level manager is operating within.
	 */
	UPROPERTY()
	UWorld* World;

	/**
	 * The level data that was unloaded & hidden prior to a new level showing, if any.
	 *
	 * Note this will never be set if the level manager is not operating in streaming mode.
	 */
	UPROPERTY()
	UVulLevelData* PreviousLevelData = nullptr;

	/**
	 * The level assets that was just shown. Can access the level's actors.
	 */
	UPROPERTY()
	ULevel* ShownLevel;

	UPROPERTY()
	FVulLevelEventContext Ctx;
};

/**
 * Describes the various states that the level manager is in with regards to loading levels.
 */
UENUM()
enum class EVulLevelManagerState : uint8
{
	/**
	 * The level manager is not actively loading any levels.
	 */
	Idle,
	
	/**
	 * A level is currently being loaded.
	 */
	Loading,
};

/**
 * Why did we fail to load a level?
 */
UENUM()
enum class EVulLevelManagerLoadFailure : uint8
{
	/**
	 * We exceeded the timeout to complete a load (locally, outside of any network considerations).
	 */
	LocalLoadTimeout,

	/**
	 * One client failed to load a level in time.
	 */
	ClientTimeout,

	/**
	 * During a network level load, some state got unexpectedly desynchronized.
	 */
	Desynchronization,
};

VULRUNTIME_API DECLARE_ENUM_TO_STRING(EVulLevelManagerLoadFailure);

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
	 *
	 * Returns true if the LoadLevel request was accepted & queued.
	 */
	bool LoadLevel(const FName& LevelName, FVulLevelDelegate::FDelegate OnComplete = FVulLevelDelegate::FDelegate());

	/**
	 * Connects a client to a remote server.
	 *
	 * Unlike other level loading methods, this does trigger a full world reload so that we can
	 * sync with the remote server's world & replication.
	 *
	 * We load the loading level whilst the connection is happening. Once complete, we'll land in
	 * the server's copy of a persistent world, then follow its ShownLevels.
	 *
	 * TODO: Needed/useful now we have UVulLevelNetworkData?
	 */
	void Connect(const FString& URI);

	/**
	 * Disconnects a client from a remote server, causing a recreation of the Root Level
	 * locally and having level manager take ownership of that.
	 *
	 * Like Connect, LoadingLevel will be shown before this reload occurs.
	 *
	 * TODO: Needed/useful now we have UVulLevelNetworkData?
	 */
	void Disconnect();

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

	void NotifyLevelLoadProgress();

	/**
	 * Returns parameters for spawning in an actor that belongs to the currently-loaded level.
	 */
	FActorSpawnParameters SpawnParams();

	/**
	 * Convenience function to spawn an actor in the given world in the current level.
	 *
	 * This exists as the UWorld::SpawnActor signatures always a bit verbose to use.
	 *
	 * SpawnParams will always have its LevelOverride set to the current level that
	 * we're showing.
	 */
	template <typename ActorType>
	ActorType* SpawnActor(
		UClass* Class,
		const FVector& Location = FVector::ZeroVector,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const TOptional<FActorSpawnParameters>& SpawnParams = {}
	) {
		FActorSpawnParameters Params;
		if (SpawnParams.IsSet())
		{
			Params = *SpawnParams;
		}
		
		SetSpawnParams(Params);

		return GetWorld()->SpawnActor<ActorType>(Class, Location, Rotation, Params);
	}

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

	/**
	 * Returns the current non-loading-screen level data that the game is playing, if any.
	 * 
	 * Returns null if we're currently loading anything.
	 */
	UVulLevelData* CurrentLevelData();

	void OnNetworkDataReplicated(AVulLevelNetworkData* NewData);

private:
	UVulLevelData* ResolveData(const FName& LevelName);

	ULevelStreaming* GetLevelStreaming(const FName& LevelName, const TCHAR* FailReason = TEXT(""));

	bool LoadLevel(
		const FName& LevelName,
		const TOptional<FString>& ServerRequestId,
		const bool Force = false,
		FVulLevelDelegate::FDelegate OnComplete = FVulLevelDelegate::FDelegate()
	);

	/**
	 * Spawns the widgets specified in LevelData, returning true if this was done.
	 */
	bool SpawnLevelWidgets(UVulLevelData* LevelData);

	void ShowLevel(const FName& LevelName);
	void HideLevel(const FName& LevelName);

	/**
	 * Initializes the level manager with the provided settings in normal operation.
	 *
	 * Will immediately load the first level, as per settings.
	 *
	 * Returns true if the level manager successfully initialized in streaming mode.
	 */
	bool InitLevelManager(const FVulLevelSettings& InSettings, UWorld* World);

	void TickNetworkHandling();

	void InitializeServerHandling();

	/**
	 * Generates a unique action info input required for streaming levels. Required for the level streaming API.
	 */
	FLatentActionInfo NextLatentAction();

	/**
	 * The last level that was successfully loaded by this manager (including loading level).
	 */
	TWeakObjectPtr<ULevelStreaming> LastLoadedLevel = nullptr;
	
	FName LastUnLoadedLevel;

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
		FString Id;
		/**
		 * If not set, a request is simply a request to unload the current level.
		 */
		TOptional<FName> LevelName;
		FVulLevelDelegate Delegate;
		TOptional<FVulTime> StartedAt;
		bool IsLoadingLevel;
		bool IsServerFollow;
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

	void NotifyActorsLevelShown(ULevel* Level);

	FVulLevelShownInfo GenerateLevelShownInfo();
	
	void SetSpawnParams(FActorSpawnParameters& Param);

	EVulLevelManagerState State = EVulLevelManagerState::Idle;

	/**
	 * Is this a server instance that clients may connect to?
	 *
	 * Enables ServerData + replication so that clients can keep in sync.
	 */
	bool IsServer() const;
	
	bool IsDedicatedServer() const;

	FVulLevelEventContext EventCtx() const;

	void FollowServer();

	/**
	 * Which level is the server showing?
	 *
	 * On the server, this is the authoritative state that we keep up to date.
	 * On the client, we have a replicated copy of this actor so we can easily track
	 *    the server moving between levels.
	 */
	UPROPERTY()
	AVulLevelNetworkData* ServerData = nullptr;
	
	UPROPERTY()
	AVulLevelNetworkData* ClientData = nullptr;

	/**
	 * Server only: mapping of network data instances to their owners.
	 *
	 * These instances are spawned on the server, but client-owned so they can
	 * notify the server of their current level state.
	 */
	UPROPERTY()
	TMap<APlayerController*, AVulLevelNetworkData*> ConnectedClients;

	float LastLoadFailLog = -1.f;

	FDelegateHandle OnClientJoined;
	FDelegateHandle OnClientLeft;

	/**
	 * Generates an ID that is used for internal logging & identification.
	 */
	FString LevelManagerNetId() const;

	/**
	 * Returns true if this is a client instance connected to & following a server.
	 *
	 * A following instance cannot make LoadLevel calls.
	 */
	bool IsFollowing() const;

	FGuid LevelManagerId;
	mutable int32 RequestIdGenerator;

	FString GenerateNextRequestId() const;

	void FailLevelLoad(const EVulLevelManagerLoadFailure Failure);
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

#define VUL_LEVEL_MANAGER_LOG(Verbosity, Format, ...) \
do { \
	UE_LOG(LogVul, Verbosity, TEXT("VulLevelManager [%s]: ") Format, *LevelManagerNetId(), ##__VA_ARGS__); \
} while (0);