#pragma once

#include "CoreMinimal.h"
#include "EngineUtils.h"

/**
 * Utility functions for AActor classes.
 *
 * Nothing complex here, just simpler APIs for common functionality when writing game actors.
 */
class VULRUNTIME_API FVulActorUtil
{
public:
	/**
	 * Spawns a component dynamically at runtime of the templated class. Sets it up so that it can be seen
	 * in the world tree/actor graph.
	 *
	 * Optionally provide a parent component to attach to, else we attach to the root component of the provided
	 * actor.
	 */
	template <typename T>
	static T* SpawnDynamicComponent(
		AActor* Owner,
		const FName& Name,
		USceneComponent* Parent   = nullptr,
		UActorComponent* Template = nullptr,
		const EObjectFlags Flags = RF_NoFlags
	) {
		return Cast<T>(SpawnDynamicComponent(T::StaticClass(), Owner, Name, Parent, Template, Flags));
	}

	/**
	 * @overload
	 *
	 * For a dynamic component class.
	 */
	static UActorComponent* SpawnDynamicComponent(
		const TSubclassOf<UActorComponent> ComponentClass,
		AActor* Owner,
		const FName& Name,
		USceneComponent* Parent   = nullptr,
		UActorComponent* Template = nullptr,
		const EObjectFlags Flags = RF_NoFlags
	);

	/**
	 * Creates a scene component that will be attached to the provided component, or attached as root if Parent
	 * is nullptr.
	 *
	 * For use in CDO constructors only.
	 *
	 * Note: Component properties referenced this way likely want UPROPERTY(VisibleAnywhere) to be modified
	 * in the editor.
	 */
	template <typename T>
	std::enable_if_t<std::is_base_of_v<USceneComponent, T>, T*>
	static ConstructorSpawnSceneComponent(AActor* Owner, const FName& Name, USceneComponent* Parent = nullptr)
	{
		auto Spawned = Owner->CreateDefaultSubobject<T>(Name);
		if (Parent != nullptr)
		{
			Spawned->SetupAttachment(Parent);
		} else
		{
			Owner->SetRootComponent(Spawned);
		}

		return Spawned;
	}

	/**
	 * Creates an actor component (non-scene-components) that will be attached to the provided actor.
	 *
	 * For use in CDO constructors only.
	 *
	 * Note: Component properties referenced this way likely want UPROPERTY(VisibleAnywhere) to be modified
	 * in the editor.
	 */
	template <typename T>
	std::enable_if_t<std::is_base_of_v<UActorComponent, T>, T*>
	static ConstructorSpawnActorComponent(AActor* Owner, const FName& Name)
	{
		return Owner->CreateDefaultSubobject<T>(Name);
	}

	template <typename T>
	std::enable_if_t<std::is_base_of_v<UActorComponent, T>, TArray<T*>>
	static GetComponentsByTag(AActor* Actor, const FName& Tag)
	{
		TArray<T*> Out;

		for (const auto& Component : Actor->GetComponentsByTag(T::StaticClass(), Tag))
		{
			Out.Add(Cast<T>(Component));
		}

		return Out;
	}

	template <typename T>
	std::enable_if_t<std::is_base_of_v<UActorComponent, T>, TArray<T*>>
	static GetComponentsByClass(AActor* Actor)
	{
		TArray<T*> Out;

		for (const auto& Component : Actor->K2_GetComponentsByClass(T::StaticClass()))
		{
			Out.Add(Cast<T>(Component));
		}

		return Out;
	}

	/**
	 * Resolves a sibling component identified by the provided class.
	 *
	 * Expects a ptr to that component which will be set if found, which is useful to avoid
	 * unnecessary work if the component has already been resolved (for example using this
	 * in a Tick function).
	 *
	 * If the component cannot be found, a warning is given and this returns false.
	 */
	template <typename ComponentClass>
	std::enable_if_t<std::is_base_of_v<UActorComponent, ComponentClass>, bool>
	static ResolveComponent(const UActorComponent* Component, ComponentClass*& Ptr)
	{
		if (IsValid(Ptr))
		{
			return true;
		}

		const auto Actor = Component->GetTypedOuter<AActor>();
		if (!ensureMsgf(IsValid(Actor), TEXT("Cannot find valid actor owning the provided component")))
		{
			return false;
		}

		return ResolveComponent(Actor, Ptr);
	}

	/**
	 * Resolves a component identified by the provided class.
	 *
	 * Expects a ptr to that component which will be set if found, which is useful to avoid
	 * unnecessary work if the component has already been resolved (for example using this
	 * in a Tick function).
	 *
	 * If the component cannot be found, a warning is given and this returns false.
	 */
	template <typename ComponentClass>
	std::enable_if_t<std::is_base_of_v<UActorComponent, ComponentClass>, bool>
	static ResolveComponent(const AActor* Actor, ComponentClass*& Ptr)
	{
		Ptr = Actor->GetComponentByClass<ComponentClass>();
		if (!ensureMsgf(
			IsValid(Ptr),
			TEXT("Could not find %s attached to %s"),
			*ComponentClass::StaticClass()->GetName(),
			*Actor->GetName()
		))
		{
			return false;
		}

		return true;
	}

	/**
	 * Returns an actor's bound box as an FBox.
	 */
	static FBox BoundingBox(AActor* Actor, const bool OnlyColliding = false, const bool IncludeChildActors = false);

	/**
	 * Finds the first actor in the provided world of the requested type, or nullptr.
	 */
	template <typename ActorType, typename = TEnableIf<TIsDerivedFrom<ActorType, AActor>::Value>>
	static ActorType* FindFirstActor(UWorld* World)
	{
		for (TActorIterator<ActorType> It(World); It; ++It)
		{
			return *It;
		}

		return nullptr;
	};
};
