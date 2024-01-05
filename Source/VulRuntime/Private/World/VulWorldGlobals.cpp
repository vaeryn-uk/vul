#include "World/VulWorldGlobals.h"

UWorld* VulRuntime::WorldGlobals::GetWorldChecked(UObject* Object)
{
	auto World = Object->GetWorld();
	checkf(IsValid(World), TEXT("Object does not have a valid world"))
	return World;
}
