#include "ActorUtil/VulComponentCollection.h"

void FVulNiagaraCollection::Init(AActor* Actor)
{
	Collection = TVulComponentCollection<UNiagaraComponent>::FromTag(Actor, TagName);
}

void FVulNiagaraCollection::Activate()
{
	Collection.ForEach([](UNiagaraComponent* Component)
	{
		Component->Activate();
	});
}

void FVulNiagaraCollection::Relocate(const FVector& WorldPos)
{
	Collection.ForEach([WorldPos](UNiagaraComponent* Component)
	{
		Component->SetWorldLocation(WorldPos);
	});
}

bool FVulNiagaraCollection::AreSystemsComplete() const
{
	return Collection.All([](UNiagaraComponent* NC)
	{
		return NC->IsComplete();
	});
}
