#include "LevelManager/VulLevelManager.h"
#include "VulRuntime.h"
#include "VulRuntimeSettings.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "LevelManager/VulLevelAwareActor.h"
#include "UserInterface/VulUserInterface.h"
#include "World/VulWorldGlobals.h"

TOptional<TPair<FName, UVulLevelData*>> FVulLevelSettings::FindLevel(UWorld* World) const
{
	for (const auto& Entry : LevelData)
	{
		if (Entry.Value.GetDefaultObject()->Level.GetAssetName() == World->GetName())
		{
			return {{Entry.Key, Entry.Value->GetDefaultObject<UVulLevelData>()}};
		}
	}

	return {};
}

bool FVulLevelSettings::IsValid() const
{
	return !LevelData.IsEmpty() && !StartingLevelName.IsNone();
}

void UVulLevelManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const auto& World = GetWorld();
	if (!IsValid(World) || !World->IsGameWorld())
	{
		return;
	}

	if (!VulRuntime::Settings()->LevelSettings.IsValid())
	{
		UE_LOG(LogVul, Display, TEXT("Skipping UVulLevelManager initialization is no LevelSettings configured."))
		return;
	}

	WorldInitDelegateHandle = World->OnLevelsChanged().AddWeakLambda(
		this,
		[this, World]
		{
			if (!IsValid(World))
			{
				return;
			}

			ensureAlwaysMsgf(
				World->OnLevelsChanged().Remove(WorldInitDelegateHandle),
				TEXT("Could not remove UVulRuntimeSubsystem world change delegate")
			);

			UE_LOG(LogVul, Display, TEXT("Initializing UVulLevelManager with configured LevelSettings."))
			InitLevelManager(VulRuntime::Settings()->LevelSettings, World);
		}
	);
}

bool UVulLevelManager::IsAllowedToTick() const
{
	return !HasAnyFlags(RF_ClassDefaultObject);
}

UVulLevelManager* VulRuntime::LevelManager(UWorld* WorldCtx)
{
	return VulRuntime::WorldGlobals::GetGameInstanceSubsystemChecked<UVulLevelManager>(WorldCtx);
}

ULevelStreaming* UVulLevelManager::GetLastLoadedLevel() const
{
	if (LastLoadedLevel.IsValid() && LastLoadedLevel->IsLevelLoaded())
	{
		return LastLoadedLevel.Get();
	}

	return nullptr;
}

void UVulLevelManager::InitLevelManager(const FVulLevelSettings& InSettings, UWorld* World)
{
	Settings = InSettings;

	const auto CurrentLevelData = Settings.FindLevel(World);

	// If the current level is not configured, we infer this is a root level containing all
	// of our game levels. Auto-load the first level.
	if (!CurrentLevelData.IsSet())
	{
		if (!Settings.LoadingLevelName.IsNone())
		{
			// If we have a loading level. Display this first.
			LoadLevel(Settings.LoadingLevelName, FVulLevelDelegate::FDelegate::CreateWeakLambda(
				this,
				[this](const UVulLevelData*, const UVulLevelManager*)
				{
					LoadLevel(Settings.StartingLevelName);
				}
			));
		} else if (!Settings.StartingLevelName.IsNone())
		{
			// Else just load the starting level without a loading screen.
			LoadLevel(Settings.StartingLevelName);
		}
	} else
	{
		// If we are starting with a level we have configured, don't mess with the level loading
		// and just invoke our extended functionality on that level.
		CurrentLevel = CurrentLevelData->Key;
		OnShowLevelData = CurrentLevelData->Value;
		bIsInStreamingMode = false;
	}
}

UVulLevelData* UVulLevelManager::ResolveData(const FName& LevelName)
{
	// Create instances if needed.
	if (Settings.LevelData.Num() != LevelDataInstances.Num())
	{
		LevelDataInstances.Reset();

		for (const auto& Entry : Settings.LevelData)
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

void UVulLevelManager::ShowLevel(const FName& LevelName)
{
	const auto ResolvedData = ResolveData(LevelName);
	if (!ensureMsgf(ResolvedData != nullptr, TEXT("ShowLevel could not resolve level %s"), *LevelName.ToString()))
	{
		return;
	}

	const auto Level = ResolvedData->Level;

	const auto LS = GetLevelStreaming(LevelName);
	if (!IsValid(LS) || GetLevelStreaming(LevelName)->GetShouldBeVisibleFlag())
	{
		// Not valid or already shown.
		return;
	}

	UE_LOG(LogVul, Display, TEXT("Showing level %s"), *LevelName.ToString())

	// Remove all widgets from the viewport from previous levels.
	RemoveAllWidgets(GetWorld());

	LastLoadedLevel = LS;
	LastLoadedLevel->SetShouldBeVisible(true);

	// Need to ensure that visibility is finalized as it seems that not all actors are
	// always available.
	GetWorld()->FlushLevelStreaming();

	Widgets.Reset();

	OnShowLevelData = ResolvedData;
}

void UVulLevelManager::HideLevel(const FName& LevelName)
{
	UE_LOG(LogVul, Display, TEXT("Hiding level %s"), *LevelName.ToString())

	if (const auto LS = GetLevelStreaming(LevelName); IsValid(LS))
	{
		LS->SetShouldBeVisible(false);
	}
}

FLatentActionInfo UVulLevelManager::NextLatentAction()
{
	FLatentActionInfo Info;
	Info.UUID = LoadingUuid++;
	return Info;
}

void UVulLevelManager::LoadAssets(const TArray<FSoftObjectPath>& Paths)
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

bool UVulLevelManager::AreWaitingForAdditionalAssets() const
{
	if (!AdditionalAssets.IsValid())
	{
		return false;
	}

	return !AdditionalAssets->HasLoadCompleted();
}

void UVulLevelManager::LoadStreamingLevel(const FName& LevelName, TSoftObjectPtr<UWorld> Level)
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

void UVulLevelManager::UnloadStreamingLevel(const FName& Name, TSoftObjectPtr<UWorld> Level)
{
	if (Name == Settings.LoadingLevelName)
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

void UVulLevelManager::RemoveAllWidgets(UWorld* World)
{
	if (!IsValid(World) || !IsValid(World->GetGameViewport()))
	{
		return;
	}

	World->GetGameViewport()->RemoveAllViewportWidgets();
}

UVulLevelManager::FLoadRequest* UVulLevelManager::CurrentRequest()
{
	if (Queue.IsValidIndex(0))
	{
		return Queue.GetData();
	}

	return nullptr;
}

void UVulLevelManager::StartProcessing(FLoadRequest* Request)
{
	Request->StartedAt = FVulTime::WorldTime(GetWorld());

	LastUnLoadedLevel = NAME_None;

	if (CurrentLevel.IsSet())
	{
		// Unload the current level.
		HideLevel(CurrentLevel.GetValue());

		const auto Current = ResolveData(CurrentLevel.GetValue());
		checkf(IsValid(Current), TEXT("Could not resolve current level object"))

		UnloadStreamingLevel(CurrentLevel.GetValue(), Current->Level);

		LastUnLoadedLevel = CurrentLevel.GetValue();
	}

	if (!Settings.LoadingLevelName.IsNone())
	{
		// Show the loading level this whilst we load.
		ShowLevel(Settings.LoadingLevelName);
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

	TArray<FSoftObjectPath> Assets;
	Data->AssetsToLoad(Assets);
	LoadAssets(Assets);
}

void UVulLevelManager::Process(FLoadRequest* Request)
{
	if (!Request->StartedAt.IsSet())
	{
		// No load in progress. Nothing to do.
		return;
	}

	if (WaitForUnload.IsSet())
	{
		const auto LS = GetLevelStreaming(WaitForUnload.GetValue());
		if (!IsValid(LS))
		{
			return;
		}

		const auto StreamingState = LS->GetLevelStreamingState();
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

	if (!Request->IsLoadingLevel && !Request->StartedAt.GetValue().IsAfter(Settings.MinimumTimeOnLoadScreen.GetTotalSeconds()))
	{
		// Loading, but haven't been on the load screen long enough.
		// Unless we're loading the loading screen, in which case go straight away.
		return;
	}

	if (Request->StartedAt.GetValue().IsAfter(Settings.LoadTimeout.GetTotalSeconds()))
	{
		// TODO: Load timed out. Then what?
		UE_LOG(LogVul, Error, TEXT("Level load timeout after %s"), *Settings.LoadTimeout.ToString());
		NextRequest();
		return;
	}

	const auto LS = GetLevelStreaming(Request->LevelName.GetValue());
	if (!IsValid(LS) || !LS->IsLevelLoaded() || AreWaitingForAdditionalAssets())
	{
		// Loading is not complete.
		return;
	}

	// Otherwise we're done. Boot it up.
	if (!Settings.LoadingLevelName.IsNone() && !Request->IsLoadingLevel)
	{
		HideLevel(Settings.LoadingLevelName);
	}

	ShowLevel(Request->LevelName.GetValue());

	UE_LOG(LogVul, Display, TEXT("Completed loading of %s"), *Request->LevelName.GetValue().ToString())

	const auto Resolved = ResolveData(Request->LevelName.GetValue());
	OnLevelLoadComplete.Broadcast(Resolved, this);
	Request->Delegate.Broadcast(Resolved, this);

	NextRequest();
}

void UVulLevelManager::NextRequest()
{
	Queue.RemoveAt(0);
}

bool UVulLevelManager::IsReloadOfSameLevel(const FName& LevelName) const
{
	if (LevelName == Settings.LoadingLevelName)
	{
		return false;
	}

	if (Queue.IsEmpty() && CurrentLevel == LevelName)
	{
		return true;
	}

	return !Queue.IsEmpty() && Queue.Last().LevelName == LevelName;
}

void UVulLevelManager::NotifyActorsLevelShown(ULevel* Level)
{
	const auto Info = GenerateLevelShownInfo();

	for (auto I = 0; I < Level->Actors.Num(); I++)
	{
		const auto Actor = Level->Actors[I];
		if (const auto LevelAware = Cast<IVulLevelAwareActor>(Actor))
		{
			LevelAware->OnVulLevelShown(Info);
		}
	}
}

FVulLevelShownInfo UVulLevelManager::GenerateLevelShownInfo()
{
	FVulLevelShownInfo Info;

	Info.World = GetWorld();
	Info.LevelManager = this;

	if (bIsInStreamingMode)
	{
		Info.ShownLevel = LastLoadedLevel->GetLoadedLevel();
		if (LastUnLoadedLevel.IsValid())
		{
			Info.PreviousLevelData = ResolveData(LastUnLoadedLevel);
		}
	} else
	{
		Info.ShownLevel = GetWorld()->GetCurrentLevel();
	}

	return Info;
}

ULevelStreaming* UVulLevelManager::GetLevelStreaming(const FName& LevelName, const TCHAR* Reason)
{
	checkf(!LevelName.IsNone(), TEXT("Invalid level name provided: "), Reason);

	const auto Data = ResolveData(LevelName);
	checkf(!Data->Level.IsNull(), TEXT("Could not find level by name %s for streaming"), *LevelName.ToString())

	const auto Loaded = UGameplayStatics::GetStreamingLevel(this, FName(*Data->Level.GetLongPackageName()));

	if (!ensureMsgf(
		Loaded != nullptr,
		TEXT("Request to load level %s failed as it was not found in the persistent level"),
		*LevelName.ToString()
	))
	{
		return nullptr;
	}

	return Loaded;
}

bool UVulLevelManager::SpawnLevelWidgets(UVulLevelData* LevelData)
{
	const auto Ctrl = VulRuntime::WorldGlobals::GetFirstPlayerController(this);
	if (!ensureMsgf(IsValid(Ctrl), TEXT("Cannot find player controller to spawn level load widgets")))
	{
		return false;
	}

	for (const auto& Widget : LevelData->Widgets)
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

	return true;
}

void UVulLevelManager::Tick(float DeltaTime)
{
	if (CurrentRequest())
	{
		if (!CurrentRequest()->StartedAt.IsSet())
		{
			// Start loading.
			StartProcessing(CurrentRequest());
		} else
		{
			Process(CurrentRequest());
		}
	}

	if (OnShowLevelData.IsValid())
	{
		if (!bIsInStreamingMode)
		{
			// Non-streaming mode: just invoke what we can globally.
			if (SpawnLevelWidgets(OnShowLevelData.Get()))
			{
				NotifyActorsLevelShown(GetWorld()->GetCurrentLevel());
				OnShowLevelData->OnLevelShown(GenerateLevelShownInfo());
				OnShowLevelData.Reset();
			}
		} else if (LastLoadedLevel.IsValid() && LastLoadedLevel->HasLoadedLevel())
		{
			// Normal, streaming mode. When the level has fully loaded and we have
			// the controller required to spawn widgets, call relevant hooks.
			if (SpawnLevelWidgets(OnShowLevelData.Get()))
			{
				NotifyActorsLevelShown(GetLastLoadedLevel()->GetLoadedLevel());
				OnShowLevelData->OnLevelShown(GenerateLevelShownInfo());
				OnShowLevelData.Reset();
			}
		}
	}
}

TStatId UVulLevelManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVulLevelManager, STATGROUP_Tickables);
}

void UVulLevelManager::LoadLevel(const FName& LevelName, FVulLevelDelegate::FDelegate OnComplete)
{
	if (!ensureAlwaysMsgf(bIsInStreamingMode, TEXT("Cannot LoadLevel() for a level manager not in streaming mode")))
	{
		return;
	}

	// Validate the level name.
	if (const auto LS = GetLevelStreaming(LevelName); !IsValid(LS))
	{
		return;
	}

	// Special case: if LevelName is the same as the level we're loading, add an unload
	// and a load entry.
	if (IsReloadOfSameLevel(LevelName))
	{
		FLoadRequest& New = Queue.AddDefaulted_GetRef();
		New.LevelName = TOptional<FName>();
	}

	FLoadRequest& New = Queue.AddDefaulted_GetRef();
	New.LevelName = LevelName;
	New.IsLoadingLevel = LevelName == Settings.LoadingLevelName;

	if (OnComplete.IsBound())
	{
		Queue.Last().Delegate.Add(OnComplete);
	}
}

FActorSpawnParameters UVulLevelManager::SpawnParams()
{
	checkf(CurrentLevel.IsSet(), TEXT("Cannot create SpawnParams as no level is loaded"))

	FActorSpawnParameters Params;
	SetSpawnParams(Params);
	return Params;
}

void UVulLevelManager::SetSpawnParams(FActorSpawnParameters& Params)
{
	checkf(CurrentLevel.IsSet(), TEXT("Cannot create SpawnParams as no level is loaded"))

	if (bIsInStreamingMode)
	{
		const auto Level = GetLevelStreaming(CurrentLevel.GetValue(), TEXT("SpawnParams"));
		if (IsValid(Level))
		{
			Params.OverrideLevel = Level->GetLoadedLevel();
		}
	}
}

