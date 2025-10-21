#include "LevelManager/VulLevelManager.h"
#include "VulRuntime.h"
#include "VulRuntimeSettings.h"
#include "ActorUtil/VulActorUtil.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "LevelManager/VulLevelAwareActor.h"
#include "LevelManager/VulLevelNetworkData.h"
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
	return !LevelData.IsEmpty() && !StartingLevelName.IsNone() && RootLevel.IsValid();
}

FString FVulLevelSettings::Summary(const bool IsDedicatedServer) const
{
	return FString::Printf(
		TEXT("Level count: %d, Root: %s, StartLevel: %s, LoadLevel: %s"),
		LevelData.Num(),
		*(RootLevel.IsValid() ? RootLevel.GetAssetName() : "none"),
		*GetStartingLevelName(IsDedicatedServer).ToString(),
		*LoadingLevelName.ToString()
	);
}

FName FVulLevelSettings::GetStartingLevelName(const bool IsDedicatedServer) const
{
	if (IsDedicatedServer && !ServerStartingLevelName.IsNone())
	{
		return ServerStartingLevelName;
	}

	return StartingLevelName;
}

void UVulLevelManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LevelManagerId = FGuid::NewGuid();

	const auto& World = GetWorld();
	if (!IsValid(World) || !World->IsGameWorld())
	{
		return;
	}

	if (!VulRuntime::Settings()->LevelSettings.IsValid())
	{
		VUL_LEVEL_MANAGER_LOG(Display, TEXT("Skipping initialization as no valid LevelSettings configured."))
		return;
	}

	// Wait until we start in the world before the level manager kicks in.
	// When trying to start right away, issues were found in non-editor builds
	// where the actual default map is not loaded when this Initialize function
	// is running.
	WorldInitDelegateHandle = FWorldDelegates::OnWorldTickStart.AddWeakLambda(
		this,
		[this](UWorld* World, ELevelTick Tick, float F)
		{
			if (!IsValid(World))
			{
				return;
			}

			VUL_LEVEL_MANAGER_LOG(
				Verbose,
				TEXT("Initializing with configured LevelSettings: %s"),
				*VulRuntime::Settings()->LevelSettings.Summary(IsDedicatedServer())
			);
			
			if (InitLevelManager(VulRuntime::Settings()->LevelSettings, World))
			{
				ensureAlwaysMsgf(
					FWorldDelegates::OnWorldTickStart.Remove(WorldInitDelegateHandle),
					TEXT("Could not remove UVulRuntimeSubsystem world change delegate")
				);
				
				VUL_LEVEL_MANAGER_LOG(
					Display,
					TEXT("Initialized with configured LevelSettings: %s"),
					*VulRuntime::Settings()->LevelSettings.Summary(IsDedicatedServer())
				);
			} else
			{
				VUL_LEVEL_MANAGER_LOG(
					Verbose,
					TEXT("Streaming initialization failed. Listening for further world starts to try again...")
				);
			}
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

UVulLevelData* UVulLevelManager::CurrentLevelData()
{
	if (!CurrentLevel.IsSet() || State != EVulLevelManagerState::Idle)
	{
		return nullptr;
	}

	return ResolveData(CurrentLevel.GetValue());
}

void UVulLevelManager::OnNetworkDataReplicated(AVulLevelNetworkData* NewData)
{
	if (!IsServer() && NewData->HasAuthority())
	{
		ClientData = NewData;
	}
}

bool UVulLevelManager::InitLevelManager(const FVulLevelSettings& InSettings, UWorld* World)
{
	Settings = InSettings;

	const auto CurrentLevelData = Settings.FindLevel(World);

	// If this matches our configured root level, start streaming stuff in.
	if (World == Settings.RootLevel.Get())
	{
		// Reset from previous attempts. We'll unset if issues.
		bIsInStreamingMode = true;
		
		VUL_LEVEL_MANAGER_LOG(
			Verbose,
			TEXT("Detected running in root level. Attempting level streaming management")
		)
		
		const auto StartingLevel = Settings.GetStartingLevelName(IsDedicatedServer());
		bool Ok = false;
		
		if (!Settings.LoadingLevelName.IsNone())
		{
			// If we have a loading level. Display this first.
			Ok = LoadLevel(Settings.LoadingLevelName, FVulLevelDelegate::FDelegate::CreateWeakLambda(
				this,
				[this, StartingLevel](const UVulLevelData*, const UVulLevelManager*)
				{
					LoadLevel(StartingLevel);
				}
			));
		} else if (!StartingLevel.IsNone())
		{
			// Else just load the starting level without a loading screen.
			Ok = LoadLevel(StartingLevel);
		}

		if (Ok)
		{
			VUL_LEVEL_MANAGER_LOG(
				Verbose,
				TEXT("Level streaming management successfully enabled")
			)
			
			return true;
		}

		VUL_LEVEL_MANAGER_LOG(Verbose, TEXT("Could not queue initial LoadLevel request"))
		bIsInStreamingMode = false;
		return false;
	}

	VUL_LEVEL_MANAGER_LOG(
		Verbose,
		TEXT("Detected running in non-root level. Disabling VulLevelManager level streaming management")
	)

	if (CurrentLevelData.IsSet())
	{
		VUL_LEVEL_MANAGER_LOG(
			Display,
			TEXT("Directly loaded non-root level %s. Running any LevelData hooks only once."),
			*CurrentLevelData->Key.ToString()
		)
		
		CurrentLevel = CurrentLevelData->Key;
		OnShowLevelData = CurrentLevelData->Value;
	}
	
	bIsInStreamingMode = false;
	
	return false;
}

void UVulLevelManager::TickNetworkHandling()
{
	if (IsServer())
	{
		InitializeServerHandling();

		// Keep the current level up to date.
		if (ServerData)
		{
			ServerData->CurrentLevel = CurrentLevel.IsSet() ? CurrentLevel.GetValue() : FName();
		}
	} else if (GetWorld() && !ServerData)
	{
		// Non servers are listening for a server data actor to follow.
		// TODO: A way to not spam actor iterators on tick.
		for (TActorIterator<AVulLevelNetworkData> It(GetWorld()); It; ++It)
		{
			if (!(*It)->IsServer)
			{
				continue;
			}
			
			ServerData = *It;

			ServerData->OnNetworkLevelChange.AddWeakLambda(this, [this](AVulLevelNetworkData* Data)
			{
				FollowServer();
			});

			VUL_LEVEL_MANAGER_LOG(
				Display,
				TEXT("Client detected VulLevelServerData to follow. Clearing queue & forcing load of %s"),
				ServerData->CurrentLevel
			)
			
			// Immediately load whatever level the server is on. This isn't a synchronized load,
			// given the server is like after load here, so just freely load up front to get
			// in sync.
			Queue.Reset();
			LoadLevel(ServerData->CurrentLevel, {}, true);

			break;
		}
	}
}

void UVulLevelManager::InitializeServerHandling()
{
	if (!ServerData && GetWorld())
	{
		VUL_LEVEL_MANAGER_LOG(Display, TEXT("Server spawning replicated VulNetworkLevelData"))
		FActorSpawnParameters Params;
		Params.Name = FName(TEXT("LevelManager_ServerData"));
		Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
		ServerData = GetWorld()->SpawnActor<AVulLevelNetworkData>(Params);
		ServerData->IsServer = true;

		if (!ServerData)
		{
			VUL_LEVEL_MANAGER_LOG(Error, TEXT("Server could not spawn its network data actor"))
		}
	}

	if (!OnClientJoined.IsValid())
	{
		OnClientJoined = FGameModeEvents::OnGameModePostLoginEvent().AddWeakLambda(
			this,
			[this](AGameModeBase* GameMode, APlayerController* Controller)
			{
				FActorSpawnParameters Params;
				Params.Name = FName(FString::Printf(
					TEXT("LevelManager_ClientData_%d"),
					Controller->PlayerState->GetPlayerId()
				));
				Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
				const auto ClientData = GetWorld()->SpawnActor<AVulLevelNetworkData>(Params);
				
				if (!ClientData)
				{
					VUL_LEVEL_MANAGER_LOG(Error, TEXT("Client could not spawn its network data actor"))
				} else
				{
					ClientData->SetOwner(Controller);
					VUL_LEVEL_MANAGER_LOG(
						Display,
						TEXT("Client %d joined & VulLevelNetworkData spawned"), Controller->PlayerState->GetPlayerId()
					)
					ConnectedClients.Add(Controller, ClientData);
				}
			}
		);
	}

	if (!OnClientLeft.IsValid())
	{
		OnClientLeft = FGameModeEvents::OnGameModeLogoutEvent().AddWeakLambda(
			this,
			[this](AGameModeBase* GameMode, AController* Controller)
			{
				const auto PC = Cast<APlayerController>(Controller);
				if (!IsValid(PC))
				{
					return;
				}
				
				if (ConnectedClients.Contains(PC))
				{
					VUL_LEVEL_MANAGER_LOG(
						Display,
						TEXT("Client %d left & VulLevelNetworkData removed"), PC->PlayerState->GetPlayerId()
					)
					// No need to destroy the UVulLevelNetworkData instance. Its client ownership implies destruction.
					ConnectedClients.Remove(PC);
				} else
				{
					VUL_LEVEL_MANAGER_LOG(
						Warning,
						TEXT("Client %d left & VulLevelNetworkData removed"), PC->PlayerState->GetPlayerId()
					)
				}
			}
		);
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

	VUL_LEVEL_MANAGER_LOG(Display, TEXT("Showing level %s"), *LevelName.ToString())

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
	VUL_LEVEL_MANAGER_LOG(Display, TEXT("Hiding level %s"), *LevelName.ToString())

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

	VUL_LEVEL_MANAGER_LOG(Display, TEXT("Loading %d additional assets with level"), Paths.Num());

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
	VUL_LEVEL_MANAGER_LOG(Verbose, TEXT("Requesting load of level %s"), *LevelName.ToString())

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

	VUL_LEVEL_MANAGER_LOG(Display, TEXT("Requesting unload of level %s"), *Name.ToString())

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
	VUL_LEVEL_MANAGER_LOG(
		Display,
		TEXT("StartProcessing %s"),
		*(Request->LevelName.IsSet() ? Request->LevelName.GetValue().ToString() : FString(TEXT("<Unload request>")))
	);

	Request->StartedAt = FVulTime::WorldTime(GetWorld());

	if (Request->LevelName.IsSet() && !Request->IsLoadingLevel)
	{
		if (IsServer() && IsValid(ServerData))
		{
			// Inform clients we're starting a level load.
			ServerData->PendingServerLevelRequest = FVulPendingLevelRequest{
				.RequestId = Request->Id,
				.LevelName = Request->LevelName.GetValue(),
				.IssuedAt = Request->StartedAt->Seconds(),
				.ClientsTotal = ConnectedClients.Num(),
				.ServerReady = false,
			};
		} else if (Request->IsServerFollow && IsValid(ClientData))
		{
			// We're on a client following a server load.
			ClientData->PendingClientLevelRequest = FVulPendingLevelRequest{
				.RequestId = Request->Id,
				.LevelName = Request->LevelName.GetValue(),
				.IssuedAt = GetWorld()->GetTimeSeconds(),
			};
		}
	}

	LastUnLoadedLevel = NAME_None;
	State = EVulLevelManagerState::Loading;

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

	VUL_LEVEL_MANAGER_LOG(Verbose, TEXT("Beginning loading of %s"), *LevelName.ToString())

	if (!Request->IsLoadingLevel)
	{
		WaitForUnload = CurrentLevel;
		CurrentLevel = Request->LevelName;
	}

	// Actually load the requested level.
	LoadStreamingLevel(LevelName, Data->Level);

	TArray<FSoftObjectPath> Assets;
	Data->AssetsToLoad(Assets, EventCtx());
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

	const auto LS = GetLevelStreaming(Request->LevelName.GetValue());
	if (!IsValid(LS) || !LS->IsLevelLoaded() || AreWaitingForAdditionalAssets())
	{
		// Loading is not complete.
		return;
	}

	// Check for a network-synchronized level load.
	if (IsValid(ServerData) && ServerData->PendingServerLevelRequest.IsValid())
	{
		if (IsServer() && !ServerData->PendingServerLevelRequest.IsComplete())
		{
			ServerData->PendingServerLevelRequest.ServerReady = true;
			
			int ClientsLoaded = 0;
			for (const auto& Entry : ConnectedClients)
			{
				if (!Entry.Value->PendingClientLevelRequest.IsValid())
				{
					// Client has not yet registered a follow request.
					continue;
				}
				
				if (Entry.Value->PendingClientLevelRequest.RequestId != ServerData->PendingServerLevelRequest.RequestId)
				{
					// If the client has registered a follow request, check it's what we're currently doing.
					FailLevelLoad(EVulLevelManagerLoadFailure::Desynchronization);
					return;
				}
				
				if (Entry.Value->PendingClientLevelRequest.IsComplete())
				{
					ClientsLoaded++;
				}
			}

			ServerData->PendingServerLevelRequest.ClientsLoaded = ClientsLoaded;

			if (ServerData->PendingServerLevelRequest.ClientsLoaded == ServerData->PendingServerLevelRequest.ClientsTotal)
			{
				ServerData->PendingServerLevelRequest.CompletedAt = GetWorld()->GetTimeSeconds();
			} else if (ServerData->PendingServerLevelRequest.IssuedAt + Settings.LoadTimeout.GetTotalSeconds() > GetWorld()->GetTimeSeconds()) {
				FailLevelLoad(EVulLevelManagerLoadFailure::ClientTimeout);
				return;
			} else
			{
				// Not all clients connected yet.
				return;
			}
		}
	}

	if (IsValid(ClientData) && ClientData->PendingClientLevelRequest.IsValid())
	{
		if (!IsValid(ServerData) || ServerData->PendingServerLevelRequest.RequestId != ClientData->PendingClientLevelRequest.RequestId)
		{
			FailLevelLoad(EVulLevelManagerLoadFailure::Desynchronization);
			return;
		}

		ClientData->PendingClientLevelRequest.CompletedAt = GetWorld()->GetTimeSeconds();
		ClientData->Server_UpdateClientRequest(ClientData->PendingClientLevelRequest);
	}

	if (Request->StartedAt.GetValue().IsAfter(Settings.LoadTimeout.GetTotalSeconds()))
	{
		FailLevelLoad(EVulLevelManagerLoadFailure::LocalLoadTimeout);
		return;
	}

	// Otherwise we're done. Boot it up.
	if (!Settings.LoadingLevelName.IsNone() && !Request->IsLoadingLevel)
	{
		HideLevel(Settings.LoadingLevelName);
	}

	ShowLevel(Request->LevelName.GetValue());

	VUL_LEVEL_MANAGER_LOG(
		Display,
		TEXT("Completed loading of %s%s"),
		*Request->LevelName.GetValue().ToString(),
		*(Request->IsServerFollow ? FString(TEXT(" (server follow)")) : FString())
	)

	const auto Resolved = ResolveData(Request->LevelName.GetValue());
	OnLevelLoadComplete.Broadcast(Resolved, this);
	State = EVulLevelManagerState::Idle;
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

	Info.Ctx = EventCtx();

	return Info;
}

ULevelStreaming* UVulLevelManager::GetLevelStreaming(const FName& LevelName, const TCHAR* Reason)
{
	checkf(!LevelName.IsNone(), TEXT("Invalid level name provided: "), Reason);

	const auto Data = ResolveData(LevelName);
	checkf(!Data->Level.IsNull(), TEXT("Could not find level by name %s for streaming"), *LevelName.ToString())

	const auto Loaded = UGameplayStatics::GetStreamingLevel(this, FName(*Data->Level.GetLongPackageName()));

	if (!Loaded)
	{
		// Log spam protection as this can occur a lot in PIE.
		if (LastLoadFailLog < 0 || FPlatformTime::Seconds() > LastLoadFailLog + 5.0f)
		{
			VUL_LEVEL_MANAGER_LOG(
				Warning,
				TEXT("Request to load level %s failed as it was not found in the persistent level"),
				*LevelName.ToString(),
				*GetWorld()->GetMapName(),
				*GetWorld()->GetName()
			)

			LastLoadFailLog = FPlatformTime::Seconds();
		}
		return nullptr;
	}

	return Loaded;
}

bool UVulLevelManager::SpawnLevelWidgets(UVulLevelData* LevelData)
{
	if (LevelData->Widgets.IsEmpty() || IsDedicatedServer())
	{
		return false;
	}
	
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
	TickNetworkHandling();
	
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
				OnShowLevelData->OnLevelShown(GenerateLevelShownInfo(), EventCtx());
				OnShowLevelData.Reset();
			}
		} else if (LastLoadedLevel.IsValid() && LastLoadedLevel->HasLoadedLevel())
		{
			// Normal, streaming mode. When the level has fully loaded and we have
			// the controller required to spawn widgets, call relevant hooks.
			if (SpawnLevelWidgets(OnShowLevelData.Get()))
			{
				NotifyActorsLevelShown(GetLastLoadedLevel()->GetLoadedLevel());
				OnShowLevelData->OnLevelShown(GenerateLevelShownInfo(), EventCtx());
				OnShowLevelData.Reset();
			}
		}
	}
}

TStatId UVulLevelManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVulLevelManager, STATGROUP_Tickables);
}

bool UVulLevelManager::LoadLevel(const FName& LevelName, FVulLevelDelegate::FDelegate OnComplete)
{
	return LoadLevel(LevelName, {}, false, OnComplete);
}

bool UVulLevelManager::LoadLevel(
	const FName& LevelName,
	const TOptional<FString>& ServerRequestId,
	const bool Force,
	FVulLevelDelegate::FDelegate OnComplete
) {
	if (!Force && IsFollowing() && !ServerRequestId.IsSet())
	{
		VUL_LEVEL_MANAGER_LOG(Error, TEXT("Ignoring LoadLevel() request as this level manager is following a server"))
		return false;
	}
	
	if (!bIsInStreamingMode)
	{
		VUL_LEVEL_MANAGER_LOG(Warning, TEXT("Cannot LoadLevel() for a level manager not in streaming mode"))
		return false;
	}

	// Validate the level name.
	if (const auto LS = GetLevelStreaming(LevelName); !IsValid(LS))
	{
		return false;
	}

	// Special case: if LevelName is the same as the level we're loading, add an unload
	// and a load entry.
	if (IsReloadOfSameLevel(LevelName))
	{
		FLoadRequest& New = Queue.AddDefaulted_GetRef();
		New.Id = ServerRequestId.Get(GenerateNextRequestId());
		New.LevelName = TOptional<FName>();
	}

	FLoadRequest& New = Queue.AddDefaulted_GetRef();
	New.Id = ServerRequestId.Get(GenerateNextRequestId());
	New.LevelName = LevelName;
	New.IsLoadingLevel = LevelName == Settings.LoadingLevelName;
	New.IsServerFollow = ServerRequestId.IsSet();

	if (OnComplete.IsBound())
	{
		Queue.Last().Delegate.Add(OnComplete);
	}

	return true;
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

bool UVulLevelManager::IsServer() const
{
	if (!GetWorld())
	{
		return false;
	}

	const auto NM = GetWorld()->GetNetMode();

	return NM_DedicatedServer == NM || NM_ListenServer == NM; 
}

bool UVulLevelManager::IsDedicatedServer() const
{
	if (!GetWorld())
	{
		return false;
	}

	const auto NM = GetWorld()->GetNetMode();

	return NM_DedicatedServer == NM; 
}

FVulLevelEventContext UVulLevelManager::EventCtx() const
{
	return {
		.IsDedicatedServer = IsDedicatedServer(),
	};
}

void UVulLevelManager::FollowServer()
{
	if (!ServerData || !ServerData->PendingServerLevelRequest.IsValid())
	{
		return;
	}

	// We may already be working on this request.
	const auto NewRequest = Queue.ContainsByPredicate([this](const FLoadRequest& Req)
	{
		return Req.Id == ServerData->PendingServerLevelRequest.RequestId;
	});
	
	if (!NewRequest)
	{
		return;
	}
	
	VUL_LEVEL_MANAGER_LOG(Display, TEXT("Following server to %s"), *ServerData->PendingServerLevelRequest.LevelName.ToString())
	LoadLevel(ServerData->CurrentLevel, ServerData->PendingServerLevelRequest.RequestId);
}

FString UVulLevelManager::LevelManagerNetId() const
{
	const auto ThisWorld = GetWorld();
	FString WorldIdStr = ThisWorld ? FString::FromInt(ThisWorld->GetUniqueID()) : FString(TEXT("unknown"));
	FString WorldNameStr = ThisWorld ? ThisWorld->GetName() : FString(TEXT("unknown"));
	FString MapNameStr = ThisWorld ? ThisWorld->GetMapName() : FString(TEXT("unknown"));
	
	const ENetMode WorldNetMode = ThisWorld ? ThisWorld->GetNetMode() : NM_Standalone;
	const TCHAR* NetModeStr = TEXT("Unknown");
	
	switch (WorldNetMode) {
		case NM_Client:          NetModeStr = TEXT("Client"); break;
		case NM_ListenServer:    NetModeStr = TEXT("ListenServer"); break;
		case NM_DedicatedServer: NetModeStr = TEXT("Server"); break;
		case NM_Standalone:      NetModeStr = TEXT("Standalone"); break;
		default: break;
	}

	return FString::Printf(TEXT("%s, WorldID: %s (%s - %s)"), NetModeStr, *WorldIdStr, *WorldNameStr, *MapNameStr);
}

bool UVulLevelManager::IsFollowing() const
{
	return IsValid(ServerData) && !ServerData->HasAuthority();
}

FString UVulLevelManager::GenerateNextRequestId() const
{
	return FString::Printf(TEXT("%s_%d"), *LevelManagerId.ToString(), ++RequestIdGenerator);
}

DEFINE_ENUM_TO_STRING(EVulLevelManagerLoadFailure, "VulRuntime")

void UVulLevelManager::FailLevelLoad(const EVulLevelManagerLoadFailure Failure)
{
	VUL_LEVEL_MANAGER_LOG(Error, "Level load failure: %s", *EnumToString(Failure));

	if (IsServer() && IsValid(ServerData))
	{
		ServerData->PendingServerLevelRequest = {};
	}

	if (IsValid(ClientData))
	{
		ClientData = nullptr;
	}

	Queue.Reset();
	// TODO: What now? Back to start level? Disconnect from server?
}

