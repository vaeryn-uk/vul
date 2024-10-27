#include "VulRuntimeSubsystem.h"
#include "VulRuntimeSettings.h"

void UVulRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const auto World = GetWorld();
	if (!IsValid(World) || !World->IsGameWorld())
	{
		return;
	}

	if (VulRuntime::Settings()->LevelSettings.IsValid())
	{
		LevelManager = World->SpawnActor<AVulLevelManager>(AVulLevelManager::StaticClass());
		LevelManager->VulInit(VulRuntime::Settings()->LevelSettings);
	}
}
