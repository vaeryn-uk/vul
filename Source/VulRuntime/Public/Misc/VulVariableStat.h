#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * A numeric value that can be modified with the ability to withdraw modifications
 * later on.
 *
 * This offers numeric interactions commonly found in RPGs, such as increasing some
 * attribute by X%.
 *
 * TODO: This only covers a basic implementation that demonstrates the intention & basic
 * design, but needs fleshing out.
 */
template <typename NumberType>
class TVulVariableStat
{
public:
	TVulVariableStat() = default;
	TVulVariableStat(const NumberType InBase) : Base(InBase) {};

	/**
	 * Alters the base value for this stat by a fixed amount.
	 *
	 * This cannot be withdrawn.
	 */
	void ModifyBase(const NumberType Amount, const TOptional<TPair<NumberType, NumberType>>& Clamp = {})
	{
		if (Clamp.IsSet())
		{
			Base = FMath::Clamp(Base + Amount, Clamp.GetValue().Key, Clamp.GetValue().Value);
		} else
		{
			Base += Amount;
		}
	}

	/**
	 * Modify the current stat value by a percentage amount of the base, additive.
	 *
	 * E.g. +0.05 will make the total value 1.05 of the base.
	 *
	 * The provided ID can later be passed to @see Remove to withdraw this modification.
	 */
	void AddPercent(const NumberType Amount, FGuid& Id)
	{
		Modifications.Add({Id, Amount});
	}

	/**
	 * Removes a single modification via its ID that was issued when applying the modification.
	 */
	void Remove(const FGuid& Id)
	{
		for (auto N = Modifications.Num() - 1; N >= 0; --N)
		{
			if (Modifications[N].Id == Id)
			{
				Modifications.RemoveAt(N);
				break;
			}
		}
	}

	/**
	 * Removes all modifications, returning this to its base value.
	 */
	void Reset()
	{
		Modifications.Empty();
	}

	/**
	 * Returns the current value with all modifications applied.
	 */
	float Current() const
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

private:
	struct FVulStatModification
	{
		FGuid Id;
		TOptional<float> Percent;
	};

	TArray<FVulStatModification> Modifications;

	NumberType Base;
};
