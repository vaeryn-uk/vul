#include "LevelManager/VulLevelManager.h"
#include "VulRuntime.h"
#include "ActorUtil/VulActorUtil.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "World/VulWorldGlobals.h"

AVulLevelManager::AVulLevelManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

AVulLevelManager* AVulLevelManager::Get(UWorld* World)
{
	return FVulActorUtil::FindFirstActor<AVulLevelManager>(World);
}

// Called when the game starts or when spawned
void AVulLevelManager::BeginPlay()
{
	Super::BeginPlay();

	if (!LoadingLevelName.IsNone())
	{
		// If we have a loading level. Display this first.
		LoadLevel(LoadingLevelName, FVulLevelDelegate::FDelegate::CreateWeakLambda(this, [this](const UVulLevelData*)
		{
			LoadLevel(StartingLevelName);
		}));
	} else if (!StartingLevelName.IsNone())
	{
		// Else just load the starting level without a loading screen.
		LoadLevel(StartingLevelName);
	} else
	{
		UE_LOG(LogVul, Warning, TEXT("No starting level set in VulLevelManager"))
	}
}

UVulLevelData* AVulLevelManager::ResolveData(const FName& LevelName)
{
	// Create instances if needed.
	if (LevelData.Num() != LevelDataInstances.Num())
	{
		LevelDataInstances.Reset();

		for (const auto& Entry : LevelData)
		{
			LevelDataInstances.Add(Entry.Key, NewObject<UVulLevelData>(this, Entry.Value));
		}
	}

	const auto Found = LevelDataInstances.Find(LevelName);

	if (Found == nullptr)
	{
		return nullptr;
	}

	return *Found;
}

void AVulLevelManager::ShowLevel(const FName& LevelName)
{
	const auto ResolvedData = ResolveData(LevelName);
	if (!ensureMsgf(ResolvedData != nullptr, TEXT("ShowLevel could not resolve level %s"), *LevelName.ToString()))
	{
		return;
	}

	const auto Level = ResolvedData->Level;

	UE_LOG(LogVul, Display, TEXT("Showing level %s"), *LevelName.ToString())

	// Remove all widgets from the viewport from previous levels.
	RemoveAllWidgets(GetWorld());

	GetLevelStreaming(LevelName)->SetShouldBeVisible(true);

	// Need to ensure that visibility is finalized as it seems that not all actors are
	// always available.
	GetWorld()->FlushLevelStreaming();

	// Spawn any widgets defined for this level.
	for (const auto& Widget : ResolvedData->Widgets)
	{
		const auto Ctrl = VulRuntime::WorldGlobals::GetFirstPlayerController(this);
		if (!ensureMsgf(IsValid(Ctrl), TEXT("Cannot find player controller to spawn level load widgets")))
		{
			continue;
		}
		const auto SpawnedWidget = CreateWidget(Ctrl, Widget.Widget.LoadSynchronous());
		if (!ensureMsgf(IsValid(SpawnedWidget), TEXT("Failed to spawn level widget")))
		{
			continue;
		}

		SpawnedWidget->AddToViewport(Widget.ZOrder);
	}

	ResolvedData->OnLevelShown();
}

void AVulLevelManager::HideLevel(const FName& LevelName)
{
	UE_LOG(LogVul, Display, TEXT("Hiding level %s"), *LevelName.ToString())

	GetLevelStreaming(LevelName)->SetShouldBeVisible(false);
}

FLatentActionInfo AVulLevelManager::NextLatentAction()
{
	FLatentActionInfo Info;
	Info.UUID = LoadingUuid++;
	return Info;
}

void AVulLevelManager::LoadAssets(const TArray<FSoftObjectPath>& Paths)
{
	if (Paths.IsEmpty())
	{
		return;
	}

	UE_LOG(LogVul, Display, TEXT("Loading %d additional assets with level"), Paths.Num());

	bIsLoadingAssets = true;
	StreamableManager.RequestAsyncLoad(Paths, FStreamableDelegate::CreateUObject(this, &AVulLevelManager::OnAssetLoaded));
}

void AVulLevelManager::OnAssetLoaded()
{
	bIsLoadingAssets = false;
}

void AVulLevelManager::LoadStreamingLevel(const FName& LevelName, TSoftObjectPtr<UWorld> Level)
{
	UE_LOG(LogVul, Display, TEXT("Requesting load of level %s"), *LevelName.ToString())

	UGameplayStatics::LoadStreamLevelBySoftObjectPtr(
		this,
		Level,
		false,
		false,
		NextLatentAction()
	);
}

void AVulLevelManager::UnloadStreamingLevel(const FName& Name, TSoftObjectPtr<UWorld> Level)
{
	if (Name == LoadingLevelName)
	{
		// We never unload our loading level.
		return;
	}

	UE_LOG(LogVul, Display, TEXT("Requesting unload of level %s"), *Name.ToString())

	UGameplayStatics::UnloadStreamLevelBySoftObjectPtr(
		this,
		Level,
		NextLatentAction(),
		false
	);
}

void AVulLevelManager::RemoveAllWidgets(UWorld* World)
{
	if (!IsValid(World) || !IsValid(World->GetGameViewport()))
	{
		return;
	}

	World->GetGameViewport()->RemoveAllViewportWidgets();
}

AVulLevelManager::FLoadRequest* AVulLevelManager::CurrentRequest()
{
	if (Queue.IsValidIndex(0))
	{
		return Queue.GetData();
	}

	return nullptr;
}

void AVulLevelManager::StartProcessing(FLoadRequest* Request)
{
	const auto LevelName = Request->LevelName;
	const auto Data = ResolveData(LevelName);
	if (!ensureMsgf(Data != nullptr, TEXT("Invalid level name request for load: %s"), *LevelName.ToString()))
	{
		return;
	}

	UE_LOG(LogVul, Display, TEXT("Beginning loading of %s"), *LevelName.ToString())

	Request->StartedAt = FVulTime::WorldTime(GetWorld());

	if (CurrentLevel.IsSet())
	{
		// Unload the current level.
		HideLevel(CurrentLevel.GetValue());

		const auto Current = ResolveData(CurrentLevel.GetValue());
		checkf(IsValid(Current), TEXT("Could not resolve current level object"))

		UnloadStreamingLevel(CurrentLevel.GetValue(), Current->Level);
	}

	if (!Request->IsLoadingLevel)
	{
		if (!LoadingLevelName.IsNone())
		{
			// If we're not loading the loading level itself, and we have a loading level, show this whilst we load.
			ShowLevel(LoadingLevelName);
		}

		PreviousLevel = CurrentLevel;
		CurrentLevel = LevelName;
	}

	// Actually load the requested level.
	LoadStreamingLevel(LevelName, Data->Level);

	LoadAssets(Data->AssetsToLoad());
}

void AVulLevelManager::Process(FLoadRequest* Request)
{
	if (!Request->StartedAt.IsSet())
	{
		// No load in progress. Nothing to do.
		return;
	}

	if (!Request->IsLoadingLevel && !Request->StartedAt.GetValue().IsAfter(MinimumTimeOnLoadScreen.GetTotalSeconds()))
	{
		// Loading, but haven't been on the load screen long enough.
		// Unless we're loading the loading screen, in which case go straight away.
		return;
	}

	if (Request->StartedAt.GetValue().IsAfter(LoadTimeout.GetTotalSeconds()))
	{
		// TODO: Load timed out. Then what?
		UE_LOG(LogVul, Error, TEXT("Level load timeout after %s"), *LoadTimeout.ToString());
		NextRequest();
		return;
	}

	if (!GetLevelStreaming(Request->LevelName)->IsLevelLoaded() || bIsLoadingAssets)
	{
		// Loading is not complete.
		return;
	}

	if (PreviousLevel.IsSet())
	{
		const auto StreamingState = GetLevelStreaming(PreviousLevel.GetValue())->GetLevelStreamingState();
		if (StreamingState != ELevelStreamingState::Unloaded && StreamingState != ELevelStreamingState::Removed)
		{
			// Previous level unload is not complete.
			return;
		}

		// Completely cleanup the previous world.
		if (const auto Resolved = ResolveData(PreviousLevel.GetValue()); Resolved->Level.IsValid())
		{
			Resolved->Level->DestroyWorld(true);
		}
	}

	// Otherwise we're done. Boot it up.
	PreviousLevel.Reset();

	if (!LoadingLevelName.IsNone() && !Request->IsLoadingLevel)
	{
		HideLevel(LoadingLevelName);
	}

	ShowLevel(Request->LevelName);

	UE_LOG(LogVul, Display, TEXT("Completed loading of %s"), *Request->LevelName.ToString())

	const auto Resolved = ResolveData(Request->LevelName);
	OnLevelLoadComplete.Broadcast(Resolved);
	Request->Delegate.Broadcast(Resolved);

	NextRequest();
}

void AVulLevelManager::NextRequest()
{
	Queue.RemoveAt(0);
}

ULevelStreaming* AVulLevelManager::GetLevelStreaming(const FName& LevelName, const TCHAR* Reason)
{
	checkf(!LevelName.IsNone(), TEXT("Invalid level name provided: "), Reason);

	const auto Data = ResolveData(LevelName);
	checkf(!Data->Level.IsNull(), TEXT("Could not find level by name %s for streaming"))

	const auto Loaded = UGameplayStatics::GetStreamingLevel(this, FName(*Data->Level.GetLongPackageName()));

	checkf(
		Loaded != nullptr,
		TEXT("Request to load level %s failed as it was not found in the persistent level"),
		*LevelName.ToString()
	)

	return Loaded;
}

// Called every frame
void AVulLevelManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CurrentRequest())
	{
		return;
	}

	if (!CurrentRequest()->StartedAt.IsSet())
	{
		// Start loading.
		StartProcessing(CurrentRequest());
	} else
	{
		Process(CurrentRequest());
	}
}

void AVulLevelManager::LoadLevel(const FName& LevelName)
{
	FLoadRequest& New = Queue.AddDefaulted_GetRef();
	New.LevelName = LevelName;
	New.IsLoadingLevel = LevelName == LoadingLevelName;
}

void AVulLevelManager::LoadLevel(const FName& LevelName, FVulLevelDelegate::FDelegate OnComplete)
{
	LoadLevel(LevelName);
	Queue.Last().Delegate.Add(OnComplete);
}

