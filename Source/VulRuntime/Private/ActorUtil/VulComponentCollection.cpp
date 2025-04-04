﻿#include "ActorUtil/VulComponentCollection.h"

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

void FVulNiagaraCollection::Deactivate()
{
	Collection.ForEach([](UNiagaraComponent* Component)
	{
		Component->Deactivate();
	});
}

void FVulNiagaraCollection::DeactivateImmediate()
{
	Collection.ForEach([](UNiagaraComponent* Component)
	{
		Component->DeactivateImmediate();
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

const TVulComponentCollection<UNiagaraComponent>& FVulNiagaraCollection::GetCollection() const
{
	return Collection;
}
