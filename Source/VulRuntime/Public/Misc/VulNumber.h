#pragma once

#include "CoreMinimal.h"
#include "VulObjectWatches.h"
#include "UObject/Object.h"

/**
 * Describes a single modification to a TVulNumber.
 */
template <typename NumberType>
struct TVulNumberModification
{
	FGuid Id;

	static TVulNumberModification MakePercent(const float InPercent, const FGuid& Id = FGuid())
	{
		TVulNumberModification Out;
		Out.Percent = InPercent;
		Out.Id = Id;
		return Out;
	}

	static TVulNumberModification MakeFlat(const NumberType Flat, const FGuid& Id = FGuid())
	{
		TVulNumberModification Out;
		Out.Flat = Flat;
		Out.Id = Id;
		return Out;
	}

	static TVulNumberModification MakeBasePercent(const float InBasePercent, const FGuid& Id = FGuid())
	{
		TVulNumberModification Out;
		Out.BasePercent = InBasePercent;
		Out.Id = Id;
		return Out;
	}

	TOptional<TPair<NumberType, NumberType>> Clamp;
	TOptional<float> Percent;
	TOptional<float> BasePercent;
	TOptional<NumberType> Flat;
};

/**
 * A numeric value that can be modified with the ability to withdraw modifications
 * later on.
 *
 * This offers numeric interactions commonly found in RPGs, such as increasing some
 * attribute by X%.
 *
 * TODO: This only covers a basic implementation that demonstrates the intention & basic
 * design, but needs fleshing out. And tests.
 */
template <typename NumberType>
class TVulNumber
{
public:
	typedef TPair<TSharedPtr<TVulNumber>, TSharedPtr<TVulNumber>> FClamp;

	TVulNumber() = default;
	TVulNumber(const NumberType InBase) : Base(InBase), Clamp({nullptr, nullptr}) {};
	TVulNumber(const NumberType InBase, FClamp InClamp) : Base(InBase), Clamp(InClamp) {};

	/**
	 * Applies a modification.
	 */
	void Modify(const TVulNumberModification<NumberType>& Modification)
	{
		WithWatch([&] { Modifications.Add(Modification); });
	}

	/**
	 * Alters the base value for this stat by a fixed amount.
	 *
	 * This cannot be withdrawn.
	 */
	void ModifyBase(const NumberType Amount, const TOptional<TPair<NumberType, NumberType>>& InClamp = {})
	{
		WithWatch([&]
		{
			if (InClamp.IsSet())
			{
				Base = FMath::Clamp(Base + Amount, InClamp.GetValue().Key, InClamp.GetValue().Value);
			} else
			{
				Base += Amount;
			}
		});
	}

	/**
	 * Returns the current value clamped between min & max.
	 */
	NumberType GetClamped(const NumberType Min, const NumberType Max) const
	{
		return FMath::Clamp(Value(), Min, Max);
	}

	/**
	 * Removes a single modification via its ID that was issued when applying the modification.
	 */
	void Remove(const FGuid& Id)
	{
		WithWatch([&]
		{
			for (auto N = Modifications.Num() - 1; N >= 0; --N)
			{
				if (Modifications[N].Id == Id)
				{
					Modifications.RemoveAt(N);
					break;
				}
			}
		});
	}

	/**
	 * Removes all modifications, returning this to its base value.
	 */
	void Reset()
	{
		WithWatch([&] { Modifications.Empty(); });
	}

	/**
	 * Returns the current value with all modifications applied.
	 */
	float Value() const
	{
		auto Out = Base;

		for (auto Modification : Modifications)
		{
			if (Modification.Percent.IsSet())
			{
				Out += Modification.Percent.GetValue() * Base;
			}
		}

		return Out;
	}

	typedef TVulObjectWatches<NumberType> WatchCollection;
	void Watch(const UObject* Obj, const typename WatchCollection::Signature& Fn) const
	{
		Watches.Add(Obj, Fn);
	}

private:
	void WithWatch(const TFunction<void ()>& Fn)
	{
		const auto Old = Value();
		Fn();
		Watches.Invoke(Value(), Old);
	}

	TArray<TVulNumberModification<NumberType>> Modifications;

	NumberType Base;

	FClamp Clamp;

	mutable WatchCollection Watches;
};
