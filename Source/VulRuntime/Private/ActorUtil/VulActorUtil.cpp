#include "ActorUtil/VulActorUtil.h"

UActorComponent* FVulActorUtil::SpawnDynamicComponent(
	const TSubclassOf<UActorComponent> ComponentClass,
	AActor* Owner,
	const FName& Name,
	USceneComponent* Parent,
	UActorComponent* Template,
	const EObjectFlags Flags
) {
	auto Spawned = NewObject<UActorComponent>(Owner, ComponentClass.Get(), Name, Flags, Template);

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

FBox FVulActorUtil::BoundingBox(AActor* Actor, const bool OnlyColliding, const bool IncludeChildActors)
{
	FVector Origin;
	FVector Extent;
	Actor->GetActorBounds(OnlyColliding, Origin, Extent, IncludeChildActors);

	return FBox::BuildAABB(Origin, Extent);
}
