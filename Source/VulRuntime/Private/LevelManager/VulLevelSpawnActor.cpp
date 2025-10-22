#include "LevelManager/VulLevelSpawnActor.h"

#include "Net/UnrealNetwork.h"

bool FVulLevelSpawnActorParams::ShouldSpawnOnClient() const
{
	return Network == EVulLevelSpawnActorNetOwnership::Local || Network == EVulLevelSpawnActorNetOwnership::ClientLocal;
}

bool FVulLevelSpawnActorParams::ShouldSpawnOnServer() const
{
	return Network == EVulLevelSpawnActorNetOwnership::Server
		|| Network == EVulLevelSpawnActorNetOwnership::Local
		|| Network == EVulLevelSpawnActorNetOwnership::Client;
}

UVulLevelActorComponent::UVulLevelActorComponent()
{
	SetIsReplicatedByDefault(true);
}

void UVulLevelActorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UVulLevelActorComponent, OwningLevelManager);
}
