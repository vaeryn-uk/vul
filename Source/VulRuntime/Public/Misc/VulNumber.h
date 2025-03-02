#pragma once

#include "CoreMinimal.h"
#include "VulObjectWatches.h"
#include "UObject/Object.h"

/**
 * Modification IDs by default are FGuids.
 */
struct TVulNumberDefaultIdStrategy {
	static FGuid Get() { return FGuid::NewGuid(); }
};

/**
 * Describes a single modification to a TVulNumber.
 *
 * The type of modification ID can be changed, but must implement equality operators.
 * ModificationIDs are optional to callers, so we provide a DefaultIdGenerator that can provide
 * alternative default IDs if FGuids are not used.
 */
template <typename NumberType, typename ModificationId = FGuid, typename DefaultIdGenerator = TVulNumberDefaultIdStrategy>
struct TVulNumberModification
{
	ModificationId Id;

	bool operator==(const TVulNumberModification& Other) const
	{
		return Id == Other.Id
			&& Flat == Other.Flat
			&& Percent == Other.Percent
			&& BasePercent == Other.BasePercent
			&& Set == Other.Set
			&& Clamp == Other.Clamp
		;
	}

	/**
	 * Modifies a number by a percentage. E.g. 1.1 increases a value by 10%.
	 */
	static TVulNumberModification MakePercent(const float InPercent, const ModificationId& Id = DefaultIdGenerator::Get())
	{
		TVulNumberModification Out;
		Out.Percent = InPercent;
		Out.Id = Id;
		return Out;
	}

	/**
	 * Modifies a number by a flat amount; this is simply added to the value.
	 */
	static TVulNumberModification MakeFlat(const NumberType Flat, const ModificationId& Id = DefaultIdGenerator::Get())
	{
		TVulNumberModification Out;
		Out.Flat = Flat;
		Out.Id = Id;
		return Out;
	}

	/**
	 * A modification that simply sets the value to the provided amount (ignoring whatever the current value is).
	 */
	static TVulNumberModification MakeSet(const NumberType Amount, const ModificationId& Id = DefaultIdGenerator::Get())
	{
		TVulNumberModification Out;
		Out.Set = Amount;
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
	static TVulNumberModification MakeBasePercent(const float InBasePercent, const ModificationId& Id = DefaultIdGenerator::Get())
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

	FVulFieldSet VulFieldSet() const
	{
		FVulFieldSet FieldSet;
		
		FieldSet.Add(FVulField::Create(&Clamp), "clamp");
		FieldSet.Add(FVulField::Create(&Percent), "pct");
		FieldSet.Add(FVulField::Create(&BasePercent), "basePct");
		FieldSet.Add(FVulField::Create(&Flat), "flat");
		FieldSet.Add(FVulField::Create(&Set), "set");
		FieldSet.Add(FVulField::Create(&Id), "id");
		
		return FieldSet;
	}

	TOptional<TPair<NumberType, NumberType>> Clamp;
	TOptional<float> Percent;
	TOptional<float> BasePercent;
	TOptional<NumberType> Flat;
	TOptional<NumberType> Set;
};

/**
 * A numeric value with support for RPG-like operations.
 *
 * - Supports modification which are tracked separately, applied in order, and can be withdrawn or overwritten
 *   independently.
 * - A base value that is maintained independently of any modifications; modifications are applied on top of the base.
 * - Ability to clamp the value with another TVulNumber for dynamic bound setting. Clamps are applied against the
 *   base and when calculating all modifications.
 * - Access to WatchCollection for registering callbacks for when the number is changed through any means.
 *
 * Note that you may want to consider TVulCharacterStat as a simpler replacement for this implementation
 * if you are dealing with RPG stats in your game.
 */
template <typename NumberType, typename ModificationId = FGuid, typename DefaultIdGenerator = TVulNumberDefaultIdStrategy>
class TVulNumber
{
public:
	typedef TPair<TSharedPtr<TVulNumber>, TSharedPtr<TVulNumber>> FClamp;
	using FModification = TVulNumberModification<NumberType, ModificationId, DefaultIdGenerator>;

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
		, Clamp({MakeShared<TVulNumber>(ClampMin), MakeShared<TVulNumber>(ClampMax)}) {}

	TVulNumber(const TVulNumber& Other)
	{
		Base = Other.Base;
		Modifications = Other.Modifications;
		Clamp = Other.Clamp;
		// Don't copy watches.
	}

	NumberType GetBase() const { return Base; }

	/**
	 * Describes the effect of a modification that's been applied.
	 */
	struct FModificationResult
	{
		/**
		 * The ID of the modification that was applied, if any. This can be used to revoke the change
		 * later.
		 */
		ModificationId Id;
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
			return After - Before;
		}

		/**
		 * True if the modification was actually applied.
		 *
		 * This is false when overwriting an existing modification (matched on ID) with the
		 * same effect modification.
		 *
		 * Note: this may be true even if the effective change is zero, as modifications can be
		 * applied that don't actually change the result value (e.g. +0.0000001% on a small int
		 * value).
		 */
		bool WasApplied;
	};

	struct FModificationInfo
	{
		NumberType Change;
		TOptional<ModificationId> Id = {};
	};

	/**
	 * Breakdowns the current number by its base + modifications.
	 *
	 * Each entry is described as a single modification, returned in-order starting with the base which
	 * reports its change assuming a start from 0.
	 */
	TArray<FModificationInfo> Breakdown() const
	{
		TArray<FModificationInfo> Out;

		Calculate(&Out);

		return Out;
	}

	FVulFieldSet VulFieldSet() const
	{
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Base), "base");
		Set.Add(FVulField::Create(&Clamp), "clamp");
		Set.Add(FVulField::Create(&Modifications), "modifications");
		Set.Add<NumberType>([&] { return Value(); }, "value");
		return Set;
	}

	/**
	 * Applies a modification that can later be revoked.
	 */
	FModificationResult Modify(const FModification& Modification)
	{
		const auto ExistingIndex = Modifications.IndexOfByPredicate([&](
				const FModification& Candidate
			)
			{
				return Candidate.Id == Modification.Id;
			}
		);

		// Short-circuit if the modification we're overriding is identical to the existing one.
		if (ExistingIndex != INDEX_NONE && Modification == Modifications[ExistingIndex])
		{
			return {
				.Id = Modification.Id,
				.Before = Value(),
				.After = Value(),
				.WasApplied = false,
			};
		}
		
		auto Result = Set([&]
		{
			if (ExistingIndex != INDEX_NONE)
			{
				Modifications.RemoveAt(ExistingIndex);
			}
			
			Modifications.Add(Modification);
		});
		
		Result.Id = Modification.Id;
		
		return Result;
	}

	/**
	 * Shorthand which applies a flat modification, returning the modification ID.
	 */
	FModificationResult Modify(const NumberType Amount)
	{
		return Modify(FModification::MakeFlat(Amount));
	}

	/**
	 * Alters the base value for this number by a fixed amount.
	 *
	 * This is permanent and cannot be withdrawn nor reset like normal modifications can be.
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
	void Remove(const ModificationId& Id)
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
		return Calculate();
	}

	typedef TVulObjectWatches<NumberType> WatchCollection;
	WatchCollection& Watch() const
	{
		return Watches;
	}

private:
	NumberType Calculate(TArray<FModificationInfo>* ModificationInfo = nullptr) const
	{
		auto Out = Base;

		if (ModificationInfo != nullptr)
		{
			ModificationInfo->Add(FModificationInfo{
				.Change = Base,
			});
		}

		for (auto Modification : Modifications)
		{
			auto New = Out;
			auto Old = Out;

			if (Modification.Percent.IsSet())
			{
				New = Modification.Percent.GetValue() * Out;
			} else if (Modification.Flat.IsSet())
			{
				New = Modification.Flat.GetValue() + Out;
			} else if (Modification.BasePercent.IsSet())
			{
				New = Out + Modification.BasePercent.GetValue() * Base;
			} else if (Modification.Set.IsSet())
			{
				New = Modification.Set.GetValue();
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

			if (ModificationInfo != nullptr)
			{
				ModificationInfo->Add(FModificationInfo{
					.Change = New - Old,
					.Id = Modification.Id,
				});
			}
		}

		return ApplyClamps(Out);
	}
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
			.WasApplied = true,
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

	TArray<FModification> Modifications;

	NumberType Base;

	FClamp Clamp;

	mutable WatchCollection Watches;
};
