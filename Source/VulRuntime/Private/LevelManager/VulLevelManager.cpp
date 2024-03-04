#include "LevelManager/VulLevelManager.h"
#include "VulRuntime.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "World/VulWorldGlobals.h"

AVulLevelManager::AVulLevelManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AVulLevelManager::BeginPlay()
{
	Super::BeginPlay();

	if (!StartingLevelName.IsNone())
	{
		LoadLevel(StartingLevelName);
	} else
	{
		UE_LOG(LogVul, Warning, TEXT("No starting level set in VulLevelManager"))
	}
}

const UVulLevelData* AVulLevelManager::ResolveData(const FName& LevelName)
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
	UE_LOG(LogVul, Display, TEXT("Showing level %s"), *Level.GetAssetName())

	// Remove all widgets from the viewport from previous levels.
	UWidgetLayoutLibrary::RemoveAllWidgets(this);

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
}

void AVulLevelManager::HideLevel(const FName& LevelName)
{
	UE_LOG(LogVul, Display, TEXT("Hiding level %s"), *ResolveData(LevelName)->Level.GetAssetName())

	GetLevelStreaming(LevelName)->SetShouldBeVisible(false);
}

FLatentActionInfo AVulLevelManager::NextLatentAction()
{
	FLatentActionInfo Info;
	Info.UUID = LoadingUuid++;
	return Info;
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
		*Data->Level.GetAssetName()
	)

	return Loaded;
}

// Called every frame
void AVulLevelManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!LevelLoadStartedAt.IsSet())
	{
		// No load in progress. Nothing to do.
		return;
	}

	if (!LevelLoadStartedAt.GetValue().IsAfter(MinimumTimeOnLoadScreen.GetTotalSeconds()))
	{
		// Loading, but haven't been on the load screen long enough.
		return;
	}

	if (LevelLoadStartedAt.GetValue().IsAfter(LoadTimeout.GetTotalSeconds()))
	{
		// TODO: Load timed out. Then what?
		UE_LOG(LogVul, Error, TEXT("Level load timeout"));
		return;
	}

	if (!GetLevelStreaming(CurrentLevel.GetValue())->IsLevelLoaded())
	{
		// Loading is not complete.
		return;
	}

	if (PreviousLevel.IsSet() &&
		(GetLevelStreaming(PreviousLevel.GetValue())->GetLevelStreamingState() != ELevelStreamingState::Unloaded
			&& GetLevelStreaming(PreviousLevel.GetValue())->GetLevelStreamingState() != ELevelStreamingState::Removed))
	{
		// Previous level unload is not complete.
		return;
	}

	// Completely cleanup the previous world.
	if (PreviousLevel.IsSet())
	{
		ResolveData(PreviousLevel.GetValue())->Level->DestroyWorld(true);
	}

	// Otherwise we're done. Boot it up.
	LevelLoadStartedAt.Reset();
	PreviousLevel.Reset();

	if (!LoadingLevelName.IsNone())
	{
		HideLevel(LoadingLevelName);
	}

	ShowLevel(CurrentLevel.GetValue());

	UE_LOG(LogVul, Display, TEXT("Completed loading of %s"), *CurrentLevel->ToString())

	OnLevelLoadComplete.Broadcast(ResolveData(CurrentLevel.GetValue()));
}

void AVulLevelManager::LoadLevel(const FName& LevelName)
{
	if (LevelLoadStartedAt.IsSet())
	{
		UE_LOG(LogVul, Error, TEXT("Ignoring request to load level as a level is already being loaded"))
		return;
	}

	const auto Data = ResolveData(LevelName);
	if (!ensureMsgf(Data != nullptr, TEXT("Invalid level name request for load: %s"), *LevelName.ToString()))
	{
		return;
	}

	UE_LOG(LogVul, Display, TEXT("Beginning loading of %s"), *Data->Level.GetAssetName())

	LevelLoadStartedAt = FVulTime::WorldTime(GetWorld());

	if (CurrentLevel.IsSet())
	{
		// Unload the current level.
		HideLevel(CurrentLevel.GetValue());

		const auto Current = ResolveData(CurrentLevel.GetValue());
		checkf(IsValid(Current), TEXT("Could not resolve current level object"))

		UGameplayStatics::UnloadStreamLevelBySoftObjectPtr(
			this,
			Current->Level,
			NextLatentAction(),
			false
		);
	}

	PreviousLevel = CurrentLevel;
	CurrentLevel  = LevelName;

	if (!LoadingLevelName.IsNone())
	{
		ShowLevel(LoadingLevelName);
	}

	// Load the requested level.
	UGameplayStatics::LoadStreamLevelBySoftObjectPtr(
		this,
		Data->Level,
		false,
		false,
		NextLatentAction()
	);
}

