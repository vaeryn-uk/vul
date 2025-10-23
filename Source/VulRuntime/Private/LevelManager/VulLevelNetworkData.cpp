#include "LevelManager/VulLevelNetworkData.h"
#include "LevelManager/VulLevelManager.h"
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
	DOREPLIFETIME(AVulLevelNetworkData, PendingServerLevelRequest);
	DOREPLIFETIME(AVulLevelNetworkData, IsServer);
	DOREPLIFETIME(AVulLevelNetworkData, ServerSpawnedClientActors);
}

void AVulLevelNetworkData::PostNetInit()
{
	Super::PostNetInit();

	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		if (const auto LM = GetWorld()->GetGameInstance()->GetSubsystem<UVulLevelManager>(); IsValid(LM))
		{
			LM->OnNetworkDataReplicated(this);
		}
	}
}

void AVulLevelNetworkData::SetPendingClientLevelRequest(const FVulPendingLevelRequest& New)
{
	PendingClientLevelRequest = New;
	Server_UpdateClientRequest(New);
}

void AVulLevelNetworkData::Server_UpdateClientRequest_Implementation(const FVulPendingLevelRequest& Request)
{
	PendingClientLevelRequest = Request;
}

void AVulLevelNetworkData::OnRep_StateChange()
{
	OnNetworkLevelChange.Broadcast(this);
}
