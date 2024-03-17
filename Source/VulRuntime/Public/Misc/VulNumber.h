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
 * - A base value that is maintained independently of any modifications; modifications are applied on top of the base.
 * - Ability to clamp the value with another TVulNumber for dynamic bound setting. Clamps are applied against the
 *   base and when calculating all modifications.
 * - Access to WatchCollection for registering callbacks for when the number is changed through any means.
 */
template <typename NumberType>
class TVulNumber
{
public:
	typedef TPair<TSharedPtr<TVulNumber>, TSharedPtr<TVulNumber>> FClamp;

	/**
	 * Helper to create a clamp definition where either boundary is optional and expressed
	 * as a single number.
	 */
	static FClamp MakeClamp(const TOptional<NumberType>& Min, const TOptional<NumberType>& Max = {})
	{
		TSharedPtr<TVulNumber> MinValue = nullptr;
		TSharedPtr<TVulNumber> MaxValue = nullptr;

		if (Min.IsSet())
		{
			MinValue = MakeShared<TVulNumber>(Min.GetValue());
		}

		if (Max.IsSet())
		{
			MaxValue = MakeShared<TVulNumber>(Max.GetValue());
		}

		return FClamp(MinValue, MaxValue);
	}

	TVulNumber() = default;
	TVulNumber(const NumberType InBase) : Base(InBase), Clamp({nullptr, nullptr}) {};
	TVulNumber(const NumberType InBase, FClamp InClamp) : Base(InBase), Clamp(InClamp) {};
	TVulNumber(const NumberType InBase, const NumberType ClampMin, const NumberType ClampMax)
		: Base(InBase)
		, Clamp({MakeShared<TVulNumber>(ClampMin), MakeShared<TVulNumber>(ClampMax)}) {};

	/**
	 * Describes the effect of a modification that's been applied.
	 */
	struct FModificationResult
	{
		/**
		 * The ID of the modification that was applied, if any. This can be used to revoke the change
		 * later.
		 */
		FGuid Id;
		/**
		 * The raw value before the modification is applied.
		 */
		NumberType Before;
		/**
		 * The raw value after the modification was applied.
		 */
		NumberType After;

		/**
		 * The raw delta this modification caused.
		 */
		NumberType Change() const
		{
			return Before - After;
		}
	};

	/**
	 * Applies a modification that can later be revoked.
	 */
	FModificationResult Modify(const TVulNumberModification<NumberType>& Modification)
	{
		auto Result = Set([&] { Modifications.Add(Modification); });
		Result.Id = Modification.Id;
		return Result;
	}

	/**
	 * Shorthand which applies a flat modification, returning the modification ID.
	 */
	FModificationResult Modify(const NumberType Amount)
	{
		return Modify(TVulNumberModification<NumberType>::MakeFlat(Amount));
	}

	/**
	 * Alters the base value for this number by a fixed amount.
	 *
	 * This is permanent and cannot be withdrawn nor reset, as with normal modifications.
	 *
	 * This is intended for characters' stats, e.g. a base stat increase, that other modifications
	 * are applied on top of.
	 */
	void ModifyBase(const NumberType Amount)
	{
		Set([&] { Base += Amount; });
	}

	/**
	 * Removes a single modification via its ID that was issued when applying the modification.
	 */
	void Remove(const FGuid& Id)
	{
		Set([&]
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
		Set([&] { Modifications.Empty(); });
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

			Out = ApplyClamps(Out);
		}

		return ApplyClamps(Out);
	}

	typedef TVulObjectWatches<NumberType> WatchCollection;
	WatchCollection& Watch() const
	{
		return Watches;
	}

private:
	/**
	 * Consistently applies a change to the value. Enforces clamps and triggers watches.
	 */
	FModificationResult Set(const TFunction<void ()>& Fn)
	{
		const auto Old = Value();
		Fn();
		Base = ApplyClamps(Base);
		const auto New = Value();
		Watches.Invoke(New, Old);

		return {
			.Before = Old,
			.After =  New,
		};
	}

	NumberType ApplyClamps(const NumberType Value) const
	{
		if (Clamp.Key.IsValid() && Clamp.Value.IsValid())
		{
			return FMath::Clamp(Value, Clamp.Key->Value(), Clamp.Value->Value());
		}

		if (Clamp.Key.IsValid())
		{
			return FMath::Max(Value, Clamp.Key->Value());
		}

		if (Clamp.Value.IsValid())
		{
			return FMath::Min(Value, Clamp.Value->Value());
		}

		return Value;
	}

	TArray<TVulNumberModification<NumberType>> Modifications;

	NumberType Base;

	FClamp Clamp;

	mutable WatchCollection Watches;
};
