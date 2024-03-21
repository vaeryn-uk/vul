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
	 */
	void Add(const UObject* Obj, const Signature& Fn)
	{
		Fns.Add({
			.Valid = [Obj] { return IsValid(Obj); },
			.Fn = Fn,
		});
	}

	/**
	 * Adds a function that isn't interested in the values; just wants to trigger on any change.
	 */
	void Add(const UObject* Obj, TFunction<void ()> Fn)
	{
		Fns.Add({
			.Valid = [Obj] { return IsValid(Obj); },
			.Fn = [Fn](int, int)
			{
				Fn();
			},
		});
	}

	/**
	 * Adds a function to this collection, bound for as long as the Valid function returns true.
	 *
	 * Will be called everytime Invoke is called, until Valid returns false in which case the
	 * Watch will be removed.
	 */
	void Add(const TFunction<bool ()> Valid, const Signature& Fn)
	{
		Fns.Add({.Valid = Valid, .Fn = Fn});
	}

	void Invoke(Type New, Type Old)
	{
		for (auto N = Fns.Num() - 1; N >= 0; --N)
		{
			if (!Fns[N].Valid())
			{
				Fns.RemoveAt(N);
			} else
			{
				Fns[N].Fn(New, Old);
			}
		}
	}

private:
	struct FVulObjectWatchFn
	{
		TFunction<bool ()> Valid;
		Signature Fn;
	};
	TArray<FVulObjectWatchFn> Fns;
};
