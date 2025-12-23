#include "LevelManager/VulLevelManager.h"
#include "VulRuntime.h"
#include "VulRuntimeSettings.h"
#include "ActorUtil/VulActorUtil.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "LevelManager/VulLevelAwareActor.h"
#include "LevelManager/VulLevelNetworkData.h"
#include "UserInterface/VulUserInterface.h"
#include "World/VulWorldGlobals.h"

DEFINE_ENUM_TO_STRING(EVulLevelManagerState, "VulRuntime");

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
	return !LevelData.IsEmpty() && !StartingLevelName.IsNone() && !RootLevel.IsNull();
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

int32 UVulLevelManager::LoadingUuid = 0;

void UVulLevelManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const auto& World = GetWorld();
	if (!IsValid(World) || !World->IsGameWorld() || HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	LevelManagerId = FGuid::NewGuid();

	if (!VulRuntime::Settings()->LevelSettings.IsValid())
	{
		VUL_LEVEL_MANAGER_LOG(
			Display,
			TEXT("Skipping initialization as no valid LevelSettings configured. Settings: %s"),
			*VulRuntime::Settings()->LevelSettings.Summary(IsDedicatedServer())
		)
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
				VUL_LEVEL_MANAGER_LOG(
					Display,
					TEXT("Initialized with configured LevelSettings: %s"),
					*VulRuntime::Settings()->LevelSettings.Summary(IsDedicatedServer())
				);
				
				ensureAlwaysMsgf(
					FWorldDelegates::OnWorldTickStart.Remove(WorldInitDelegateHandle),
					TEXT("Could not remove UVulRuntimeSubsystem world change delegate")
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

bool UVulLevelManager::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject);
}

UVulLevelManager* VulRuntime::LevelManager(UWorld* WorldCtx)
{
	return VulRuntime::WorldGlobals::GetGameInstanceSubsystemChecked<UVulLevelManager>(WorldCtx);
}

void UVulLevelManager::ForEachPlayer(const FVulPlayerConnectionEvent::FDelegate& OnAdded)
{
	if (HasLocalPlayer())
	{
		// We're playing too!
		OnAdded.Execute(GetLocalPlayerController());
	}
	
	for (const auto& Entry : ConnectedClients)
	{
		OnAdded.Execute(Entry.Key);
	}

	OnPlayerConnected.Add(OnAdded);
}

TArray<APlayerController*> UVulLevelManager::GetPlayers() const
{
	TArray<APlayerController*> Out;
	
	if (HasLocalPlayer())
	{
		// We're playing too!
		Out.Add(GetLocalPlayerController());
	}
	
	for (const auto& Entry : ConnectedClients)
	{
		Out.Add(Entry.Key);
	}

	return Out;
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
	if (!IsValid(NewData)) return;

	// Only followers (clients). Server should never take this path.
	if (NewData->HasAuthority())
	{
		return;
	}

	// If you mean "this replicated object corresponds to *my* local player"
	const APlayerController* PC = GetLocalPlayerController();
	if (IsValid(PC) && NewData->GetOwner() == PC)
	{
		VUL_LEVEL_MANAGER_LOG(Verbose, TEXT("Received new network data belonging to us - how we inform the server of our state"));
		FollowerData = NewData;
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
#if WITH_EDITOR
	if (IsFollower() && FollowerData)
	{
		FollowerData->SetPendingClientLevelManagerId(LevelManagerNetInfo());
	}

	if (IsPrimary() && PrimaryData)
	{
		PrimaryData->LevelManagerId = LevelManagerNetInfo();
	}
#endif
	
	if (IsFollower())
	{
		if (IsDisconnectedFromServer())
		{
			 // Disconnection detected. Don't follow anymore.
			PrimaryData = nullptr;
			FollowerData = nullptr;
			return;
		}
	}
	
	if (IsPrimary())
	{
		InitializePrimaryHandling();

		// Keep the current level up to date.
		if (PrimaryData)
		{
			PrimaryData->CurrentLevel = CurrentLevel.IsSet() ? CurrentLevel.GetValue() : FName();
		}
	} else if (GetWorld() && !PrimaryData && !IsDisconnectedFromServer())
	{
		// Non servers are listening for a server data actor to follow.
		// TODO: A way to not spam actor iterators on tick.
		for (TActorIterator<AVulLevelNetworkData> It(GetWorld()); It; ++It)
		{
			if (!(*It)->IsServer)
			{
				continue;
			}

			VUL_LEVEL_MANAGER_LOG(Verbose, TEXT("Client detected server network data. Binding & following..."))
			
			PrimaryData = *It;
			
			PrimaryData->OnNetworkLevelChange.AddWeakLambda(this, [this](AVulLevelNetworkData* Data)
			{
				FollowServer();
			});

			// Try follow immediately. Will switch to whatever level the server is already on.
			FollowServer();

			break;
		}
	}

	if (PrimaryData)
	{
		for (int I = PendingFollowerActors.Num() - 1; I >= 0; I--)
		{
			for (const auto& Entry : PrimaryData->ServerSpawnedClientActors)
			{
				if (!Entry.IsValid())
				{
					continue;
				}
				
				if (Entry.Actor->IsA(PendingFollowerActors[I].Actor) && Entry.Actor->GetOwner() == GetController() && !LevelActors.Contains(Entry))
				{
					RegisterLevelActor(Entry);
					PendingFollowerActors.RemoveAt(I);
					break;
				}
			}
		}
	}
}

void UVulLevelManager::InitializePrimaryHandling()
{
	if (!PrimaryData && GetWorld())
	{
		VUL_LEVEL_MANAGER_LOG(Display, TEXT("Server spawning replicated VulNetworkLevelData"))
		FActorSpawnParameters Params;
		Params.Name = FName(TEXT("LevelManager_ServerData"));
		Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
		PrimaryData = GetWorld()->SpawnActor<AVulLevelNetworkData>(Params);
		PrimaryData->IsServer = true;

		if (!PrimaryData)
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

				OnPlayerConnected.Broadcast(Controller);

				if (const auto LD = CurrentLevelData(); LD)
				{
					SpawnLevelActorsPerPlayer(LD->GetActorsToSpawn(EventCtx()), Controller);
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

				OnPlayerDisconnected.Broadcast(PC);
				
				if (ConnectedClients.Contains(PC))
				{
					VUL_LEVEL_MANAGER_LOG(
						Display,
						TEXT("Client %d left & VulLevelNetworkData removed"), PC->PlayerState->GetPlayerId()
					)
					// No need to destroy the UVulLevelNetworkData instance. Its client ownership implies destruction.
					ConnectedClients.Remove(PC);
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

UVulLevelData* UVulLevelManager::ResolveData(const FLoadRequest* Request)
{
	if (Request && !Request->LevelName->IsNone())
	{
		return ResolveData(Request->LevelName.GetValue());
	}

	return nullptr;
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
	if (!IsValid(LS))
	{
		// Not valid or already shown.
		VUL_LEVEL_MANAGER_LOG(
			Warning,
			TEXT("Not showing level %s because LS was invalid"),
			*LevelName.ToString()
		)
		return;
	}

	VUL_LEVEL_MANAGER_LOG(Display, TEXT("Showing level %s"), *LevelName.ToString())

	LastLoadedLevel = LS;
	LastLoadedLevel->SetShouldBeVisible(true);

	// Need to ensure that visibility is finalized as it seems that not all actors are
	// always available.
	GetWorld()->FlushLevelStreaming();

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
		TEXT("StartProcessing %s%s (requestId=%s)"),
		*(Request->LevelName.IsSet() ? Request->LevelName.GetValue().ToString() : FString(TEXT("<Unload request>"))),
		*(Request->IsServerFollow ? FString(TEXT(" (server follow)")) : FString()),
		*(Request->Id)
	);

	Request->StartedAt = FVulTime::PlatformTime();
	
	if (IsValid(FollowerData))
	{
		// Clear any previous pending request state.
		FollowerData->SetPendingClientLevelRequest({});
	}

	if (Request->LevelName.IsSet() && !Request->IsLoadingLevel)
	{
		if (IsPrimary())
		{
			// Inform clients we're starting a level load.
			PrimaryData->PendingPrimaryLevelRequest = FVulPendingLevelRequest{
				.RequestId = Request->Id,
				.LevelName = Request->LevelName.GetValue(),
				.IssuedAt = Request->StartedAt->Seconds(),
				.ClientsTotal = ConnectedClients.Num(),
				.ServerReady = false,
			};
		} else if (IsValid(FollowerData))
		{
			// We're on a client following a server load.
			FollowerData->SetPendingClientLevelRequest(FVulPendingLevelRequest{
				.RequestId = Request->Id,
				.LevelName = Request->LevelName.GetValue(),
				.IssuedAt = GetWorld()->GetTimeSeconds(),
				.CompletedAt = -1,
			});
		}
	}

	LastUnLoadedLevel = NAME_None;
	TransitionState(EVulLevelManagerState::Loading_Started);

	if (CurrentLevel.IsSet())
	{
		// Unload the current level.
		HideLevel(CurrentLevel.GetValue());
		RemoveLevelActors();
		
		// Any pending ShowLevelData hooks should be cleared. If they've not fired by now, it's too late.
		if (OnShowLevelData.IsValid())
		{
			VUL_LEVEL_MANAGER_LOG(
				Verbose,
				TEXT("Level %s hidden whilst OnShow hooks still pending. Invalidating hooks"),
				*CurrentLevel->ToString()
			)
			OnShowLevelData.Reset();
		}

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
		TransitionState(EVulLevelManagerState::Loading_MinimumLoadScreenTime);
		return;
	}

#define EXCEEDED_LOAD_TIMEOUT Request->StartedAt.GetValue().IsAfter(Settings.LoadTimeout.GetTotalSeconds())

	const auto LS = GetLevelStreaming(Request->LevelName.GetValue());
	if (!IsValid(LS) || !LS->IsLevelLoaded())
	{
		TransitionState(EVulLevelManagerState::Loading_StreamingInProgress);
		
		// Loading is not complete.
		if (EXCEEDED_LOAD_TIMEOUT)
		{
			FailLevelLoad(EVulLevelManagerLoadFailure::LocalLoadTimeout);
		}
		
		return;
	}

	if (AreWaitingForAdditionalAssets())
	{
		TransitionState(EVulLevelManagerState::Loading_AdditionalAssets);
		return;
	}

	if (Request->IsServerFollow && IsDisconnectedFromServer())
	{
		// If we disconnect from the server during load, hard stop.
		FailLevelLoad(EVulLevelManagerLoadFailure::Desynchronization);
		return;
	}

	// Check for a network-synchronized level load.
	if (IsValid(PrimaryData) && PrimaryData->PendingPrimaryLevelRequest.IsValid())
	{
		if (IsPrimary() && !PrimaryData->PendingPrimaryLevelRequest.IsComplete())
		{
			PrimaryData->PendingPrimaryLevelRequest.ServerReady = true;
			
			int ClientsLoaded = 0;
			for (const auto& Entry : ConnectedClients)
			{
				if (!Entry.Value->PendingClientLevelRequest.IsValid())
				{
					// Client has not yet registered a follow request.
					continue;
				}
				
				if (Entry.Value->PendingClientLevelRequest.RequestId != PrimaryData->PendingPrimaryLevelRequest.RequestId)
				{
					// If the client has registered a follow request, check it's what we're currently doing.
					FailLevelLoad(
						EVulLevelManagerLoadFailure::Desynchronization,
						FString::Printf(
							TEXT("Primary Request ID: %s, Follower Request ID: %s"),
							*PrimaryData->PendingPrimaryLevelRequest.RequestId,
							*Entry.Value->PendingClientLevelRequest.RequestId
						)
					);
					return;
				}
				
				if (Entry.Value->PendingClientLevelRequest.IsComplete())
				{
					ClientsLoaded++;
				}
			}

			PrimaryData->PendingPrimaryLevelRequest.ClientsLoaded = ClientsLoaded;

			if (PrimaryData->PendingPrimaryLevelRequest.ClientsLoaded == PrimaryData->PendingPrimaryLevelRequest.ClientsTotal)
			{
				PrimaryData->PendingPrimaryLevelRequest.CompletedAt = GetWorld()->GetTimeSeconds();
			} else if (EXCEEDED_LOAD_TIMEOUT) {
				FailLevelLoad(EVulLevelManagerLoadFailure::ClientTimeout);
				return;
			} else
			{
				// Not all clients connected yet.
				NotifyLevelLoadProgress();
				TransitionState(EVulLevelManagerState::Loading_PrimaryAwaitingFollowers);
				return;
			}
		}
	}
	
	// If we exceed time now, it's because the server hasn't loaded in time.
	if (IsFollower() && EXCEEDED_LOAD_TIMEOUT)
	{
		FailLevelLoad(EVulLevelManagerLoadFailure::ServerTimeout);
		return;
	}

	// Need to verify & tell primary that we (the follower) are ready - only if we're in a synced level load.
	if (IsValid(FollowerData) && FollowerData->PendingClientLevelRequest.IsValid() && !PrimaryData->PendingPrimaryLevelRequest.IsComplete())
	{
		if (!IsValid(PrimaryData) || PrimaryData->PendingPrimaryLevelRequest.RequestId != FollowerData->PendingClientLevelRequest.RequestId)
		{
			FailLevelLoad(
				EVulLevelManagerLoadFailure::Desynchronization,
				FString::Printf(
					TEXT("Primary Request ID: %s, Follower Request ID: %s"),
					*PrimaryData->PendingPrimaryLevelRequest.RequestId,
					*FollowerData->PendingClientLevelRequest.RequestId
				)
			);
			return;
		}

		if (!FollowerData->PendingClientLevelRequest.IsComplete())
		{
			FollowerData->PendingClientLevelRequest.CompletedAt = GetWorld()->GetTimeSeconds();
			FollowerData->SetPendingClientLevelRequest(FollowerData->PendingClientLevelRequest);
			VUL_LEVEL_MANAGER_LOG(Display, TEXT("Client-side loading complete; telling server we're ready"))
		}
	}

	if (IsFollower() && PrimaryData->PendingPrimaryLevelRequest.IsPending())
	{
		TransitionState(EVulLevelManagerState::Loading_FollowerAwaitingPrimary);
		NotifyLevelLoadProgress();
		return;
	}

	// Finally, followers waiting for their copy of an actor spawned on the server on their behalf.
	if (!PendingFollowerActors.IsEmpty())
	{
		TransitionState(EVulLevelManagerState::Loading_PendingFollowerActors);
		return;
	}

	if (EXCEEDED_LOAD_TIMEOUT)
	{
		FailLevelLoad(EVulLevelManagerLoadFailure::LocalLoadTimeout);
		return;
	}

	// Otherwise we're done. Boot it up.
	if (!Settings.LoadingLevelName.IsNone() && !Request->IsLoadingLevel)
	{
		// Queue the hiding of our loading level until OnShow logic has been invoked.
		LoadingLevelReadyToHide = true;
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
	TransitionState(EVulLevelManagerState::Idle);
	Request->Delegate.Broadcast(Resolved, this);

	NextRequest();
}

void UVulLevelManager::NextRequest()
{
	if (!Queue.IsEmpty())
	{
		Queue.RemoveAt(0);
	}
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

	for (TActorIterator<AActor> It(Info.World); It; ++It)
	{
		AActor* Actor = *It;
		if (const auto LevelAware = Cast<IVulLevelAwareActor>(Actor))
		{
			LevelAware->OnVulLevelChangeComplete(Info);
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

void UVulLevelManager::SpawnLevelWidgets(UVulLevelData* LevelData, APlayerController* Ctrl)
{
	// Clear anything from previous levels.
	RemoveAllWidgets(GetWorld());
	Widgets.Reset(); 

	if (!LevelData->Widgets.IsEmpty())
	{
		VUL_LEVEL_MANAGER_LOG(
			Display,
			TEXT("Spawning %d level-managed widgets from level data for %s"),
			LevelData->Widgets.Num(),
			*LevelData->Level->GetName()
		)
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
}

bool UVulLevelManager::SpawnLevelActors(UVulLevelData* LevelData)
{
	if (!GetWorld())
	{
		return false;
	}
	
	TArray<FVulLevelSpawnActorParams> ActorsToSpawn = LevelData->GetActorsToSpawn(EventCtx());

	if (IsPrimary())
	{
		for (const auto& Ctrl : GetPlayers())
		{
			SpawnLevelActorsPerPlayer(ActorsToSpawn, Ctrl);
		}
		
		PrimaryData->ServerSpawnedClientActors.Reset();
	}

	PendingFollowerActors.Reset();

	for (const auto& Entry : ActorsToSpawn)
	{
		FActorSpawnParameters Params;
		SetLevelSpawnParams(Params);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		TArray<FVulLevelManagerSpawnedActor> SpawnedActors = {};

		switch (Entry.Network)
		{
		case EVulLevelSpawnActorNetOwnership::Independent:
			SpawnedActors.Add(SpawnLevelActor(Entry));
			break;
		case EVulLevelSpawnActorNetOwnership::Primary:
			if (IsPrimary())
			{
				const auto Spawned = SpawnLevelActor(Entry, PrimaryActorTag);
				
				if (PrimaryData && Spawned.IsValid() && Spawned.Actor->GetIsReplicated())
				{
					PrimaryData->ServerSpawnedActors.Add(Spawned);
				}
				
				SpawnedActors.Add(Spawned);
			}
			break;
		case EVulLevelSpawnActorNetOwnership::PlayerLocal:
			if (HasLocalPlayer())
			{
				SpawnedActors.Add(SpawnLevelActor(Entry));
			}
			break;
		case EVulLevelSpawnActorNetOwnership::PerPlayer:
			if (IsFollower())
			{
				// Record that we're waiting for some server-spawned actors belonging to us
				// to replicate down.
				// Note that preserved actors are still added to this array. Even if we have
				// them already, we'll re-resolve them from the replicated server actors array.
				PendingFollowerActors.Add(Entry);
			}
			break;
		default: break;
		}

		for (const auto& SpawnEntry : SpawnedActors)
		{
			if (SpawnEntry.IsValid())
			{
				// Invoke shown hook straight away for non-level spawns as these
				// won't get picked up by later NotifyActorsLevelShown.
				if (SpawnEntry.SpawnPolicy != EVulLevelSpawnActorPolicy::SpawnLevel)
				{
					if (const auto& LAA = Cast<IVulLevelAwareActor>(SpawnEntry.Actor))
					{
						LAA->OnVulLevelShown(GenerateLevelShownInfo());
					}
				}

				RegisterLevelActor(SpawnEntry);
			}
		}
	}

	VUL_LEVEL_MANAGER_LOG(Display, TEXT("Spawned %d level-managed actors in to level"), LevelActors.Num())

	return true;
}

void UVulLevelManager::SpawnLevelActorsPerPlayer(
	const TArray<FVulLevelSpawnActorParams>& Actors,
	APlayerController* Follower
) {
	for (const auto& Actor : Actors)
	{
		if (Actor.Network == EVulLevelSpawnActorNetOwnership::PerPlayer)
		{
			if (const auto Spawned = SpawnLevelActor(Actor, LevelActorTag(Follower)); Spawned.IsValid())
			{
				Spawned.Actor->SetOwner(Follower);
				RegisterLevelActor(Spawned);

				if (PrimaryData)
				{
					PrimaryData->ServerSpawnedClientActors.Add(Spawned);
				}
			}
		}
	}
}

FVulLevelManagerSpawnedActor UVulLevelManager::SpawnLevelActor(FVulLevelSpawnActorParams Params, const FName& Tag)
{
	if (!GetWorld())
	{
		return {};
	}

	if (Params.SpawnPolicy == EVulLevelSpawnActorPolicy::SpawnRoot_Preserve)
	{
		const auto Existing = LevelActors.ContainsByPredicate([Params](const FVulLevelManagerSpawnedActor& Spawned)
		{
			return IsValid(Spawned.Actor) && Spawned.Actor->GetClass() == Params.Actor;
		});

		if (Existing)
		{
			VUL_LEVEL_MANAGER_LOG(
				Verbose,
				TEXT("Skipping spawn of %s spawnpolicy=preserved and a level actor already exists"),
				*Params.Actor->GetName()
			)
			
			return {};
		}
	}
	
	FActorSpawnParameters SpawnParams;
	
	if (Params.SpawnPolicy == EVulLevelSpawnActorPolicy::SpawnLevel)
	{
		SetLevelSpawnParams(SpawnParams);
	}
	
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const auto Spawned = GetWorld()->SpawnActor(Params.Actor, nullptr, nullptr, SpawnParams);

	if (!Tag.IsNone())
	{
		Spawned->Tags.Add(Tag);
	}

	return {
		.SpawnPolicy = Params.SpawnPolicy,
		.Actor = Spawned,
	};
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

	if (OnShowLevelData.IsValid() && GetWorld())
	{
		ULevel* LevelToTrigger = nullptr;

		if (!bIsInStreamingMode)
		{
			LevelToTrigger = GetWorld()->GetCurrentLevel();
		} else if (LastLoadedLevel.IsValid() && LastLoadedLevel->HasLoadedLevel())
		{
			LevelToTrigger = LastLoadedLevel->GetLoadedLevel();
		}

		if (IsValid(LevelToTrigger))
		{
			SpawnLevelActors(OnShowLevelData.Get());
			NotifyActorsLevelShown(LastLoadedLevel->GetLoadedLevel());
			OnShowLevelData->OnLevelShown(GenerateLevelShownInfo(), EventCtx());
			LastFailureReason = EVulLevelManagerLoadFailure::None;
			
			if (HasLocalPlayer())
			{
				SpawnLevelWidgets(OnShowLevelData.Get(), GetLocalPlayerController());
			}
			
			OnShowLevelData.Reset();

			if (bIsInStreamingMode && LoadingLevelReadyToHide)
			{
				HideLevel(Settings.LoadingLevelName);
				LoadingLevelReadyToHide = false;
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

void UVulLevelManager::Connect(const FString& URI)
{
	if (!HasLocalPlayer())
	{
		VUL_LEVEL_MANAGER_LOG(Error, TEXT("Cannot Connect() from an instance that does not have a local player"))
		return;
	}
	
	ResetLevelManager();

	LoadLevel(
		Settings.LoadingLevelName,
		FVulLevelDelegate::FDelegate::CreateWeakLambda(this, [this, URI](const UVulLevelData*, const class UVulLevelManager*)
		{
			VUL_LEVEL_MANAGER_LOG(Display, TEXT("Connecting to %s"), *URI);
			GetLocalPlayerController()->ConsoleCommand(FString::Printf(TEXT("open %s"), *URI));
			
			// TODO: Error handling/failures etc.
		})
	);
}

bool UVulLevelManager::LoadLevel(
	const FName& LevelName,
	const TOptional<FString>& ServerRequestId,
	const bool Force,
	FVulLevelDelegate::FDelegate OnComplete
) {
	if (!Force && IsFollower() && !ServerRequestId.IsSet())
	{
		VUL_LEVEL_MANAGER_LOG(Error, TEXT("Ignoring LoadLevel() request as this level manager is following a primary"))
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

void UVulLevelManager::NotifyLevelLoadProgress()
{
	if (!IsLoading(State))
	{
		return;
	}

	if (const auto LoadingLevel = ResolveData(Settings.LoadingLevelName); LoadingLevel)
	{
		LoadingLevel->OnLoadProgress(
			PrimaryData ? PrimaryData->PendingPrimaryLevelRequest : FVulPendingLevelRequest(),
			EventCtx()
		);
	}
}

FActorSpawnParameters UVulLevelManager::SpawnParams()
{
	checkf(CurrentLevel.IsSet(), TEXT("Cannot create SpawnParams as no level is loaded"))

	FActorSpawnParameters Params;
	SetLevelSpawnParams(Params);
	return Params;
}

void UVulLevelManager::SetLevelSpawnParams(FActorSpawnParameters& Params)
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
	return IsNetModeOneOf({NM_DedicatedServer, NM_ListenServer});
}

bool UVulLevelManager::IsClient() const
{
	return IsNetModeOneOf({NM_Client, NM_ListenServer, NM_Standalone});
}

bool UVulLevelManager::IsClientOnly() const
{
	return IsNetModeOneOf({NM_Client});
}

bool UVulLevelManager::IsDedicatedServer() const
{
	return IsNetModeOneOf({NM_DedicatedServer}); 
}

bool UVulLevelManager::IsNetModeOneOf(const TArray<ENetMode>& NetModes) const
{
	if (!GetWorld())
	{
		return false;
	}

	return NetModes.Contains(GetWorld()->GetNetMode());
}

bool UVulLevelManager::IsDisconnectedFromServer() const
{
	return GetWorld()
			&& GetWorld()->GetNetDriver()
			&& GetWorld()->GetNetDriver()->ServerConnection->GetConnectionState() == USOCK_Closed;
}

FVulLevelEventContext UVulLevelManager::EventCtx() const
{
	return {
		.IsDedicatedServer = IsDedicatedServer(),
		.FailureReason = LastFailureReason,
	};
}

void UVulLevelManager::FollowServer()
{
	if (!PrimaryData || PrimaryData->HasAuthority())
	{
		// Nothing to follow, or it's our own data.
		return;
	}

	FName LevelName = NAME_None;
	TOptional<FString> RequestId = {};

	if (PrimaryData->PendingPrimaryLevelRequest.IsPending())
	{
		// We may already be working on this request.
		const auto ExistingRequest = Queue.ContainsByPredicate([this](const FLoadRequest& Req)
		{
			return Req.Id == PrimaryData->PendingPrimaryLevelRequest.RequestId;
		});
	
		if (ExistingRequest)
		{
			return;
		}

		LevelName = PrimaryData->PendingPrimaryLevelRequest.LevelName;
		RequestId = PrimaryData->PendingPrimaryLevelRequest.RequestId;
	
		VUL_LEVEL_MANAGER_LOG(
			Display,
			TEXT("Following server to %s (synchronized network level switch) (RequestId=%s)"),
			*PrimaryData->PendingPrimaryLevelRequest.LevelName.ToString(),
			*RequestId.Get(FString("None"))
		)
	}

	if (LevelName.IsNone() && !PrimaryData->CurrentLevel.IsNone())
	{
		LevelName = PrimaryData->CurrentLevel;

		if (!LevelName.IsNone())
		{
			VUL_LEVEL_MANAGER_LOG(
				Display,
				TEXT("Following server to %s (server current level) (RequestId=%s)"),
				*LevelName.ToString(),
				*RequestId.Get(FString("None"))
			)
		}
	}

	if (CurrentLevel.IsSet() && CurrentLevel.GetValue() == LevelName)
	{
		VUL_LEVEL_MANAGER_LOG(Verbose, TEXT("Skipping server follow to %s as we're already there"), *LevelName.ToString());
		return;
	}

	const auto AlreadyLoading = Queue.ContainsByPredicate([LevelName](const FLoadRequest& Req)
	{
		return Req.LevelName.IsSet() && Req.LevelName == LevelName;
	});

	if (AlreadyLoading)
	{
		VUL_LEVEL_MANAGER_LOG(
			Verbose,
			TEXT("Skipping server follow to %s as we're already queued to go there"),
			*LevelName.ToString()
		);
		return;
	}

	if (!LevelName.IsNone())
	{
		LoadLevel(LevelName, RequestId, true);
	}
}

FString UVulLevelManager::LevelManagerNetInfo() const
{
	const auto ThisWorld = GetWorld();
	FString WorldIdStr = ThisWorld ? FString::FromInt(ThisWorld->GetUniqueID()) : FString(TEXT("unknown"));
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

	if (IsPrimary())
	{
		return FString::Printf(TEXT("%s (PRIMARY), World: %s (%s)"), NetModeStr, *WorldIdStr, *MapNameStr);
	}

	return FString::Printf(TEXT("%s (FOLLOWER), World: %s (%s)"), NetModeStr, *WorldIdStr, *MapNameStr);
}

bool UVulLevelManager::IsFollower() const
{
	return IsValid(PrimaryData) && !PrimaryData->HasAuthority();
}

bool UVulLevelManager::IsPrimary() const
{
	// Note that standalone builds become Client once they Connect().
	return !IsFollower() && !IsNetModeOneOf({NM_Client});
}

bool UVulLevelManager::HasLocalPlayer() const
{
	return IsValid(GetLocalPlayerController());
}

APlayerController* UVulLevelManager::GetLocalPlayerController() const
{
	if (GetWorld())
	{
		if (const auto LP = GetWorld()->GetFirstLocalPlayerFromController(); IsValid(LP))
		{
			return LP->GetPlayerController(GetWorld());
		}
	}

	return nullptr;
}

FString UVulLevelManager::GenerateNextRequestId() const
{
	return FString::Printf(TEXT("%s_%d"), *LevelManagerId.ToString(), ++RequestIdGenerator);
}

void UVulLevelManager::FailLevelLoad(const EVulLevelManagerLoadFailure Failure, const FString Detail)
{
	VUL_LEVEL_MANAGER_LOG(
		Error,
		"Level load failure: %s%s",
		*EnumToString(Failure),
		*(Detail.IsEmpty() ? "" : " " + Detail)
	);

	TransitionState(EVulLevelManagerState::Idle);

	ResetLevelManager();

	LastFailureReason = Failure;

	LoadLevel(Settings.GetStartingLevelName(IsDedicatedServer()));
}

FName UVulLevelManager::LevelActorTag(APlayerController* Controller) const
{
	if (IsPrimary() && (!Controller || Controller == GetController()))
	{
		return PrimaryActorTag;
	}

	if (!Controller)
	{
		Controller = GetController();
	}

	if (!Controller || !Controller->PlayerState)
	{
		return FName();
	}
	
	return FName(FString::Printf(TEXT("vullevelmanager_follower_actor_%d"), Controller->PlayerState->GetPlayerId()));
}

APlayerController* UVulLevelManager::GetController() const
{
	if (GetWorld() && GetWorld()->GetFirstLocalPlayerFromController())
	{
		return GetWorld()->GetFirstLocalPlayerFromController()->GetPlayerController(GetWorld());
	}

	return nullptr;
}

void UVulLevelManager::RegisterLevelActor(const FVulLevelManagerSpawnedActor& Actor)
{
	LevelActors.Add(Actor);
}

void UVulLevelManager::RemoveLevelActors(const bool Force)
{
	int Removed = 0;
	for (int32 I = LevelActors.Num() - 1; I >= 0; I--)
	{
		const auto ForRemoval = LevelActors[I];

		bool CanRemove = true;

		if (IsValid(ForRemoval.Actor) && !Force)
		{
			for (const auto& Entry : Queue)
			{
				for (const auto& Actor : ResolveData(&Entry)->GetActorsToSpawn(EventCtx()))
				{
					if (Actor.SpawnPolicy == EVulLevelSpawnActorPolicy::SpawnRoot_Preserve && Actor.Actor == ForRemoval.Actor->GetClass())
					{
						CanRemove = false;

						VUL_LEVEL_MANAGER_LOG(
							Verbose,
							TEXT("Preserving actor %s as it is preserved by upcoming level %s"),
							*ForRemoval.Actor->GetName(),
							*(Entry.LevelName.Get(FName()).ToString())
						)

						break;
					}
				}

				if (!CanRemove)
				{
					break;
				}
			}
		}

		if (CanRemove)
		{
			if (IsValid(ForRemoval.Actor))
			{
				VUL_LEVEL_MANAGER_LOG(
					Verbose,
					TEXT("Removing level actor %s"),
					*ForRemoval.Actor->GetName()
				);
			
				Removed++;
			}
			
			LevelActors.RemoveAt(I);
		}
	}

	if (Removed > 0)
	{
		VUL_LEVEL_MANAGER_LOG(
			Display,
			TEXT("Removed %d level actors%s"),
			Removed,
			Force ? TEXT(" (forced removal, ignoring actor spawn policy settings)") : TEXT("")
		);
	}
}

void UVulLevelManager::ResetLevelManager()
{
	if (IsPrimary() && IsValid(PrimaryData))
	{
		if (GetWorld() && GetWorld()->GetNetDriver())
		{
			for (const auto& Conn : GetWorld()->GetNetDriver()->ClientConnections)
			{
				if (Conn)
				{
					VUL_LEVEL_MANAGER_LOG(Display, TEXT("Disconnecting client: %s"), *Conn->LowLevelGetRemoteAddress());
					Conn->Close();
				}
			}
		}

		ConnectedClients.Reset();
		PrimaryData->PendingPrimaryLevelRequest = {};
	}

	PrimaryData = nullptr;
	FollowerData = nullptr;
	OnShowLevelData.Reset();
	
	if (OnClientJoined.IsValid())
	{
		FGameModeEvents::OnGameModePostLoginEvent().Remove(OnClientJoined);
	}
	OnClientJoined.Reset();

	if (OnClientLeft.IsValid())
	{
		FGameModeEvents::OnGameModeLogoutEvent().Remove(OnClientLeft);
	}
	OnClientLeft.Reset();

	RemoveLevelActors(true);
	Queue.Reset();
}

void UVulLevelManager::TransitionState(const EVulLevelManagerState New)
{
	if (State != New)
	{
		VUL_LEVEL_MANAGER_LOG(Verbose, TEXT("State transition: %s"), *EnumToString(New));
		State = New;
	}
}

