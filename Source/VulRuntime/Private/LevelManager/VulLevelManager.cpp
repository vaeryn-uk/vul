#include "LevelManager/VulLevelManager.h"
#include "VulRuntime.h"
#include "ActorUtil/VulActorUtil.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "LevelManager/VulLevelAwareActor.h"
#include "UserInterface/VulUserInterface.h"
#include "World/VulWorldGlobals.h"

AVulLevelManager::AVulLevelManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

AVulLevelManager* AVulLevelManager::Get(UWorld* World)
{
	return FVulActorUtil::FindFirstActor<AVulLevelManager>(World);
}

ULevelStreaming* AVulLevelManager::GetLastLoadedLevel() const
{
	if (LastLoadedLevel.IsValid() && LastLoadedLevel->IsLevelLoaded())
	{
		return LastLoadedLevel.Get();
	}

	return nullptr;
}

// Called when the game starts or when spawned
void AVulLevelManager::BeginPlay()
{
	Super::BeginPlay();

	if (!LoadingLevelName.IsNone())
	{
		// If we have a loading level. Display this first.
		LoadLevel(LoadingLevelName, FVulLevelDelegate::FDelegate::CreateWeakLambda(
			this,
			[this](const UVulLevelData*, const AVulLevelManager*)
			{
				LoadLevel(StartingLevelName);
			}
		));
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

	if (GetLevelStreaming(LevelName)->GetShouldBeVisibleFlag())
	{
		// Already shown.
		return;
	}

	UE_LOG(LogVul, Display, TEXT("Showing level %s"), *LevelName.ToString())

	// Remove all widgets from the viewport from previous levels.
	RemoveAllWidgets(GetWorld());

	LastLoadedLevel = GetLevelStreaming(LevelName);
	LastLoadedLevel->SetShouldBeVisible(true);
	bIsPendingActorOnShow = true;

	// Need to ensure that visibility is finalized as it seems that not all actors are
	// always available.
	GetWorld()->FlushLevelStreaming();

	Widgets.Reset();

	const auto Ctrl = VulRuntime::WorldGlobals::GetFirstPlayerController(this);
	if (ensureMsgf(IsValid(Ctrl), TEXT("Cannot find player controller to spawn level load widgets")))
	{
		// Spawn any widgets defined for this level.
		for (const auto& Widget : ResolvedData->Widgets)
		{
			const auto SpawnedWidget = CreateWidget(Ctrl, Widget.Widget.LoadSynchronous());
			if (!ensureMsgf(IsValid(SpawnedWidget), TEXT("Failed to spawn level widget")))
			{
				continue;
			}

			if (VulRuntime::UserInterface::AttachRootUMG(SpawnedWidget, Ctrl, Widget.ZOrder))
			{
				Widgets.Add(SpawnedWidget);
			}
		}
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

	if (AdditionalAssets.IsValid())
	{
		// Free additional assets we loaded before.
		AdditionalAssets->ReleaseHandle();
	}

	AdditionalAssets = StreamableManager.RequestAsyncLoad(Paths);
}

bool AVulLevelManager::AreWaitingForAdditionalAssets() const
{
	if (!AdditionalAssets.IsValid())
	{
		return false;
	}

	return !AdditionalAssets->HasLoadCompleted();
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
	Request->StartedAt = FVulTime::WorldTime(GetWorld());

	if (CurrentLevel.IsSet())
	{
		// Unload the current level.
		HideLevel(CurrentLevel.GetValue());

		const auto Current = ResolveData(CurrentLevel.GetValue());
		checkf(IsValid(Current), TEXT("Could not resolve current level object"))

		UnloadStreamingLevel(CurrentLevel.GetValue(), Current->Level);
	}

	if (!LoadingLevelName.IsNone())
	{
		// Show the loading level this whilst we load.
		ShowLevel(LoadingLevelName);
	}

	if (!Request->LevelName.IsSet())
	{
		// If this is just a request to unload, stop now.
		WaitForUnload = CurrentLevel;
		CurrentLevel.Reset();
		return;
	}

	const auto LevelName = Request->LevelName.GetValue();
	const auto Data = ResolveData(LevelName);
	if (!ensureMsgf(Data != nullptr, TEXT("Invalid level name request for load: %s"), *LevelName.ToString()))
	{
		return;
	}

	UE_LOG(LogVul, Display, TEXT("Beginning loading of %s"), *LevelName.ToString())

	if (!Request->IsLoadingLevel)
	{
		WaitForUnload = CurrentLevel;
		CurrentLevel = Request->LevelName;
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

	if (WaitForUnload.IsSet())
	{
		const auto StreamingState = GetLevelStreaming(WaitForUnload.GetValue())->GetLevelStreamingState();
		if (StreamingState != ELevelStreamingState::Unloaded && StreamingState != ELevelStreamingState::Removed)
		{
			// Previous level unload is not complete.
			return;
		}

		// Completely cleanup the previous world.
		if (const auto Resolved = ResolveData(WaitForUnload.GetValue()); Resolved->Level.IsValid())
		{
			Resolved->Level->DestroyWorld(true);
		}
	}

	WaitForUnload.Reset();

	if (!Request->LevelName.IsSet())
	{
		// If simply an unload request, we're done at this stage.
		NextRequest();
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

	if (!GetLevelStreaming(Request->LevelName.GetValue())->IsLevelLoaded() || AreWaitingForAdditionalAssets())
	{
		// Loading is not complete.
		return;
	}

	// Otherwise we're done. Boot it up.
	if (!LoadingLevelName.IsNone() && !Request->IsLoadingLevel)
	{
		HideLevel(LoadingLevelName);
	}

	ShowLevel(Request->LevelName.GetValue());

	UE_LOG(LogVul, Display, TEXT("Completed loading of %s"), *Request->LevelName.GetValue().ToString())

	const auto Resolved = ResolveData(Request->LevelName.GetValue());
	OnLevelLoadComplete.Broadcast(Resolved, this);
	Request->Delegate.Broadcast(Resolved, this);

	NextRequest();
}

void AVulLevelManager::NextRequest()
{
	Queue.RemoveAt(0);
}

bool AVulLevelManager::IsReloadOfSameLevel(const FName& LevelName) const
{
	if (LevelName == LoadingLevelName)
	{
		return false;
	}

	if (Queue.IsEmpty() && CurrentLevel == LevelName)
	{
		return true;
	}

	return !Queue.IsEmpty() && Queue.Last().LevelName == LevelName;
}

ULevelStreaming* AVulLevelManager::GetLevelStreaming(const FName& LevelName, const TCHAR* Reason)
{
	checkf(!LevelName.IsNone(), TEXT("Invalid level name provided: "), Reason);

	const auto Data = ResolveData(LevelName);
	checkf(!Data->Level.IsNull(), TEXT("Could not find level by name %s for streaming"), *LevelName.ToString())

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

	if (bIsPendingActorOnShow && LastLoadedLevel.IsValid() && LastLoadedLevel->HasLoadedLevel())
	{
		const auto Level = LastLoadedLevel->GetLoadedLevel();
		for (auto I = 0; I < Level->Actors.Num(); I++)
		{
			const auto Actor = Level->Actors[I];
			if (const auto LevelAware = Cast<IVulLevelAwareActor>(Actor))
			{
				LevelAware->OnVulLevelShown();
			}
		}

		bIsPendingActorOnShow = false;
	}
}

void AVulLevelManager::LoadLevel(const FName& LevelName, FVulLevelDelegate::FDelegate OnComplete)
{
	// Validate the level name.
	GetLevelStreaming(LevelName);

	// Special case: if LevelName is the same as the level we're loading, add an unload
	// and a load entry.
	if (IsReloadOfSameLevel(LevelName))
	{
		FLoadRequest& New = Queue.AddDefaulted_GetRef();
		New.LevelName = TOptional<FName>();
	}

	FLoadRequest& New = Queue.AddDefaulted_GetRef();
	New.LevelName = LevelName;
	New.IsLoadingLevel = LevelName == LoadingLevelName;

	if (OnComplete.IsBound())
	{
		Queue.Last().Delegate.Add(OnComplete);
	}
}

FActorSpawnParameters AVulLevelManager::SpawnParams()
{
	checkf(CurrentLevel.IsSet(), TEXT("Cannot create SpawnParams as no level is loaded"))
	const auto Level = GetLevelStreaming(CurrentLevel.GetValue(), TEXT("SpawnParams"));

	FActorSpawnParameters Params;
	Params.OverrideLevel = Level->GetLoadedLevel();

	return Params;
}

