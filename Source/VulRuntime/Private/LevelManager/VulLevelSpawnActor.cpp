#include "LevelManager/VulLevelSpawnActor.h"

bool FVulLevelManagerSpawnedActor::IsValid() const
{
	return ::IsValid(Actor);
}

bool FVulLevelManagerSpawnedActor::operator==(const FVulLevelManagerSpawnedActor& Other) const
{
	return SpawnPolicy == Other.SpawnPolicy && Actor == Other.Actor;
}

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
