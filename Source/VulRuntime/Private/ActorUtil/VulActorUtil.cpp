#include "ActorUtil/VulActorUtil.h"

UActorComponent* FVulActorUtil::SpawnDynamicComponent(
	const TSubclassOf<UActorComponent> ComponentClass,
	AActor* Owner,
	const FName& Name,
	USceneComponent* Parent,
	UActorComponent* Template)
{
	auto Spawned = NewObject<UActorComponent>(Owner, ComponentClass.Get(), Name, RF_NoFlags, Template);

	if (Parent == nullptr)
	{
		Parent = Owner->GetRootComponent();
	}

	Spawned->RegisterComponent();
	if (auto Scene = Cast<USceneComponent>(Spawned))
	{
		Scene->AttachToComponent(Parent, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
	}
	Owner->AddInstanceComponent(Spawned);

	return Spawned;
}
