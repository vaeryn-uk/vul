#include "Hexgrid/VulHexgridSpawn.h"
#include "ActorUtil/VulActorUtil.h"

AVulHexgridSpawn::AVulHexgridSpawn()
{
	PrimaryActorTick.bCanEverTick = false;

	FVulActorUtil::ConstructorSpawnSceneComponent<USceneComponent>(this, TEXT("Root"));
}

FVector AVulHexgridSpawn::GetSpawnLocation(UWorld* World)
{
	const auto Found = FVulActorUtil::FindFirstActor<AVulHexgridSpawn>(World);
	checkf(IsValid(Found), TEXT("No HexgridSpawn actor found in the world!"));
	return Found->GetActorLocation();
}

