#include "LevelManager/VulLevelSpawnActor.h"

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
