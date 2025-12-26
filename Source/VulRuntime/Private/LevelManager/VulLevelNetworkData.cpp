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
	DOREPLIFETIME(AVulLevelNetworkData, PendingPrimaryLevelRequest);
	DOREPLIFETIME(AVulLevelNetworkData, IsServer);
	DOREPLIFETIME(AVulLevelNetworkData, ServerSpawnedClientActors);
	DOREPLIFETIME(AVulLevelNetworkData, LevelManagerId);
}

void AVulLevelNetworkData::PostNetInit()
{
	Super::PostNetInit();

	if (const auto LM = VulRuntime::LevelManager(GetWorld()))
	{
		LM->OnNetworkDataReplicated(this);
	}
}

void AVulLevelNetworkData::SetPendingClientLevelRequest(const FVulPendingLevelRequest& New)
{
	PendingClientLevelRequest = New;
	Server_UpdateClientRequest(New);
}

void AVulLevelNetworkData::SetPendingClientLevelManagerId(const FString& Id)
{
	// Debugging only: no need to keep this up to date outside of editor.
#if WITH_EDITOR
	if (Id != LevelManagerId)
	{
		LevelManagerId = Id;
		Server_UpdatePendingLevelManagerId(Id);
	}
#endif
}

void AVulLevelNetworkData::Server_UpdateClientRequest_Implementation(const FVulPendingLevelRequest& Request)
{
	PendingClientLevelRequest = Request;
}

void AVulLevelNetworkData::Server_UpdatePendingLevelManagerId_Implementation(const FString& Id)
{
	LevelManagerId = Id;
}

void AVulLevelNetworkData::OnRep_StateChange()
{
	OnNetworkLevelChange.Broadcast(this);
}
