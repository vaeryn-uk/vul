#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "VulActorUtil.h"
#include "UObject/Object.h"
#include "VulComponentCollection.generated.h"

/**
 * A convenient API for accessing multiple components as one.
 *
 * Rather than hardcoding components in CPP classes, it is sometimes useful to have components defined on
 * blueprints in the editor. This API allows CPP to access these optional components easily.
 *
 * Components are indicated as belonging to a collection via component tags.
 */
template <typename ComponentType>
struct TVulComponentCollection
{
	/**
	 * Creates a collection of components for Actor that have the provided tag.
	 *
	 * Note the component list is generated and stored at this point; the component
	 * list will not change for the lifetime of this collection.
	 */
	static TVulComponentCollection FromTag(AActor* InActor, const FName& TagName)
	{
		TVulComponentCollection Out;

		Out.InitFromTag(InActor, TagName);

		return Out;
	}

	/**
	 * Runs the provided function on all valid components in the collection.
	 *
	 * Returns true if we execute Fn on at least one component.
	 */
	bool ForEach(const TFunction<void (ComponentType*)>& Fn) const
	{
		bool Executed = false;

		for (const auto& Component : Components)
		{
			if (Component.IsValid())
			{
				Fn(Component.Get());
				Executed = true;
			}
		}

		return Executed;
	}

	/**
	 * Returns true only if all valid components match the predicate.
	 *
	 * IfEmpty can be used to customize the result if the collection is empty.
	 */
	bool All(const TFunction<bool (ComponentType*)>& Predicate, bool IfEmpty = true) const
	{
		bool Executed = false;
		for (const auto& Component : Components)
		{
			if (Component.IsValid())
			{
				Executed = true;
				if (!Predicate(Component.Get()))
				{
					return false;
				}
			}
		}

		if (!Executed)
		{
			return IfEmpty;
		}

		return true;
	}

protected:
	void InitFromTag(AActor* Actor, const FName& TagName)
	{
		for (const auto& Component : FVulActorUtil::GetComponentsByTag<ComponentType>(Actor, TagName))
		{
			if (IsValid(Component))
			{
				Components.Add(Component);
			}
		}
	}

private:
	TArray<TWeakObjectPtr<ComponentType>> Components;
};

/**
 * A collection of niagara components belonging to an actor. Provides an API for interacting
 * with an actor's niagara components that share a tag.
 *
 * This allows you to add 0 or more niagara components to an actor and have them all activated
 * and queried at once. Niagara is a particularly good use-case for our component collections
 * because:
 *   - visual effects are often varied and non-critical to functionality, so hard-coding references
 *     in CPPs actor to a niagara component is limiting.
 *   - there is a bug (in UE where CPP-initialized niagara components can not have their parameters
 *     configured in the editor; only editor-added niagara components function correctly.
 *     https://forums.unrealengine.com/t/niagara-user-parameter-is-not-visible-in-blueprint-ue5-2-c/1179343/4
 *
 * Define these as a UPROPERTY on an actor class, and ensure that Init is called before the
 * collection is used (e.g. in BeginPlay).
 */
USTRUCT()
struct VULRUNTIME_API FVulNiagaraCollection
{
	GENERATED_BODY()

	/**
	 * This collection will contain all niagara systems that have this tag on your actor.
	 */
	UPROPERTY(EditAnywhere)
	FName TagName;

	/**
	 * Initializes this collection based on the components belonging to Actor.
	 */
	void Init(AActor* Actor);

	/**
	 * Activate all niagara systems in this collection.
	 */
	void Activate();

	/**
	 * De-activate all niagara systems in this collection, waiting for particle systems to complete.
	 */
	void Deactivate();

	/**
	 * De-activate all niagara systems in this collection immediately.
	 *
	 * Forcefully kills particle systems.
	 */
	void DeactivateImmediate();

	/**
	 * Moves all niagara systems in this collection to specified world position.
	 */
	void Relocate(const FVector& WorldPos);

	/**
	 * Returns true if all niagara systems in this collection are complete.
	 *
	 * Returns true if this collection is empty.
	 */
	bool AreSystemsComplete() const;

	const TVulComponentCollection<UNiagaraComponent>& GetCollection() const;

private:

	TVulComponentCollection<UNiagaraComponent> Collection;
};