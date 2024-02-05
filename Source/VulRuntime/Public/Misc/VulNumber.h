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

	/**
	 * Modifies a number by a percentage. E.g. 1.1 increases a value by 10%.
	 */
	static TVulNumberModification MakePercent(const float InPercent, const FGuid& Id = FGuid())
	{
		TVulNumberModification Out;
		Out.Percent = InPercent;
		Out.Id = Id;
		return Out;
	}

	/**
	 * Modifies a number by a flat amount; this is simple added to the value.
	 */
	static TVulNumberModification MakeFlat(const NumberType Flat, const FGuid& Id = FGuid())
	{
		TVulNumberModification Out;
		Out.Flat = Flat;
		Out.Id = Id;
		return Out;
	}

	/**
	 * Modifies a number by a percent of the base. This is added to the value.
	 *
	 * E.g.
	 *   -  +0.2 increases the value by 20% of the base amount.
	 *   -  -1.0 decreases the value by 100% of the base amount.
	 */
	static TVulNumberModification MakeBasePercent(const float InBasePercent, const FGuid& Id = FGuid())
	{
		TVulNumberModification Out;
		Out.BasePercent = InBasePercent;
		Out.Id = Id;
		return Out;
	}

	/**
	 * Sets an upper and lower limit for the amount of difference to the final value this modification
	 * can have.
	 *
	 * This behaves the same for all types of modification: calculate the difference that the modification
	 * would cause, then clamp it before applying.
	 */
	TVulNumberModification& WithClamp(const NumberType Min, const NumberType Max)
	{
		Clamp = {Min, Max};

		return *this;
	}

	TOptional<TPair<NumberType, NumberType>> Clamp;
	TOptional<float> Percent;
	TOptional<float> BasePercent;
	TOptional<NumberType> Flat;
};

/**
 * A numeric value with support for RPG-like operations.
 *
 * - Supports modification which are tracked separately, applied in order, and can be withdrawn independently.
 * - A base value that is tracked independently of any modifications.
 * - Ability to clamp the value with another TVulNumber for dynamic bound setting.
 * - Access to WatchCollection for registering callbacks for when the number is changed through any means.
 */
template <typename NumberType>
class TVulNumber
{
public:
	typedef TPair<TSharedPtr<TVulNumber>, TSharedPtr<TVulNumber>> FClamp;

	TVulNumber() = default;
	TVulNumber(const NumberType InBase) : Base(InBase), Clamp({nullptr, nullptr}) {};
	TVulNumber(const NumberType InBase, FClamp InClamp) : Base(InBase), Clamp(InClamp) {};
	TVulNumber(const NumberType InBase, const NumberType ClampMin, const NumberType ClampMax)
		: Base(InBase)
		, Clamp({MakeShared<TVulNumber>(ClampMin), MakeShared<TVulNumber>(ClampMax)}) {};

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
	 * This is permanent and cannot be withdrawn nor reset, as with normal modifications.
	 */
	void ModifyBase(const NumberType Amount)
	{
		WithWatch([&] { Base += Amount; });
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
	NumberType Value() const
	{
		auto Out = Base;

		for (auto Modification : Modifications)
		{
			auto New = Out;

			if (Modification.Percent.IsSet())
			{
				New = Modification.Percent.GetValue() * Out;
			} else if (Modification.Flat.IsSet())
			{
				New = Modification.Flat.GetValue() + Out;
			} else if (Modification.BasePercent.IsSet())
			{
				New = Out + Modification.BasePercent.GetValue() * Base;
			}

			if (Modification.Clamp.IsSet())
			{
				const auto Diff = New - Out;
				New = FMath::Clamp(Diff, Modification.Clamp.GetValue().Key, Modification.Clamp.GetValue().Value);
				Out += New;
			} else
			{
				Out = New;
			}
		}

		if (Clamp.Key.IsValid() && Clamp.Value.IsValid())
		{
			return FMath::Clamp(Out, Clamp.Key->Value(), Clamp.Value->Value());
		}

		return Out;
	}

	typedef TVulObjectWatches<NumberType> WatchCollection;
	WatchCollection& Watch() const
	{
		return Watches;
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
