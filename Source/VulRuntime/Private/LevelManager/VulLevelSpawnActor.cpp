#include "LevelManager/VulLevelSpawnActor.h"

bool FVulLevelManagerSpawnedActor::IsValid() const
{
	return ::IsValid(Actor);
}

bool FVulLevelManagerSpawnedActor::operator==(const FVulLevelManagerSpawnedActor& Other) const
{
	return SpawnPolicy == Other.SpawnPolicy && Actor == Other.Actor;
}
