#include "LevelManager/VulLevelNetworkData.h"
#include "Net/UnrealNetwork.h"

AVulLevelNetworkData::AVulLevelNetworkData()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AVulLevelNetworkData::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVulLevelNetworkData, CurrentLevel);
}

void AVulLevelNetworkData::OnRep_CurrentLevel()
{
	OnNetworkLevelChange.Broadcast(this);
}
