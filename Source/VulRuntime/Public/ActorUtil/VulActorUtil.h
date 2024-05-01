#pragma once

#include "CoreMinimal.h"

/**
 * Utility functions for AActor classes.
 *
 * Nothing complex here, just simpler APIs for common functionality when writing game actors.
 */
class VULRUNTIME_API FVulActorUtil
{
public:
	/**
	 * Spawns a scene component dynamically at runtime of the templated class. Sets it up so that it can be seen
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
		UActorComponent* Template = nullptr)
	{
		return Cast<T>(SpawnDynamicComponent(T::StaticClass(), Owner, Name, Parent, Template));
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
		UActorComponent* Template = nullptr);

	/**
	 * Creates a component that will be attached to the provided component, or attached as root if Parent
	 * is nullptr.
	 *
	 * For use in CDO constructors only.
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
	 * Returns an actor's bound box as an FBox.
	 */
	static FBox BoundingBox(AActor* Actor, const bool OnlyColliding = false, const bool IncludeChildActors = false);
};
