#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * A collection of watch functions that live for as long as their owning UObject.
 *
 * The signature of the functions take a New value and an Old value. Designed for situations where
 * you want to watch for changes in a value.
 */
template <typename Type>
class TVulObjectWatches
{
public:
	using Signature = TFunction<void (const Type New, const Type Old)>;

	/**
	* Adds a function to this collection, bound to the lifetime of the provided UObject.
	*
	* Will be called everytime Invoke is called (if its object is still valid).
	*
	* TODO: Should we provide a lifetime callback to detangle this from UObject? Then could provide
	*   an IsValid(Obj) implementation of this.
	*/
	void Add(const UObject* Obj, const Signature& Fn)
	{
		if (!Fns.Contains(Obj))
		{
			Fns.Add(Obj, TArray{Fn});
		} else
		{
			Fns.Find(Obj)->Add(Fn);
		}
	}

	void Invoke(Type New, Type Old)
	{
		// TODO: Detect and remove invalid callbacks.
		// TODO: Test coverage.
		for (auto ToCall : Fns)
		{
			for (auto Fn : ToCall.Value)
			{
				Fn(New, Old);
			}
		}
	}

private:
	TMap<TWeakObjectPtr<const UObject>, TArray<Signature>> Fns;
};
