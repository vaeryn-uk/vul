#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * A character stat implements a single value which represents a character's proficiency
 * in some characteristic. This is common in RPGs.
 *
 * This class manages modifying the stat in a controlled manner, bucketing modifications by
 * source such they can be withdrawn, or apply modifications that only effect certain sources.
 *
 * This implementation is a second attempt of TVulNumber, but with reduced functionality and
 * a focus on simplicity.
 */
template <typename NumberType, typename SourceType>
class TVulCharacterStat
{
public:
	TVulCharacterStat() = default;

	/**
	 * Constructs a new stat with an initial value, with an optional min and max clamp.
	 *
	 * Note that clamps are only applied when retrieving via Value(). Internally, the stat
	 * may reach much higher/lower than the specified clamps.
	 */
	TVulCharacterStat(
		const NumberType Initial,
		const TOptional<NumberType>& Min = {},
		const TOptional<NumberType>& Max = {}
	) : bIsValid(true), Base(Initial), ClampMin(Min), ClampMax(Max) {}

	/**
	 * Clamps the total contribution that can be made by a single source.
	 *
	 * Unlike Base, this clamp is applied on modification, rather than retrieval, so Delta
	 * to a bucket that is already at its bound(s) will do nothing.
	 */
	void Clamp(const SourceType Source, const TOptional<NumberType>& Min, const TOptional<NumberType>& Max)
	{
		BucketConfig.Add(Source, FBucketConfig{.Min = Min, .Max = Max});
	}

	/**
	 * Is this a valid stat, or from the default constructor?
	 */
	bool IsValid() const { return bIsValid; }

	/**
	 * Gets the stat's base value, ignoring modification from any sources.
	 */
	NumberType GetBase() const { return Base; }

	/**
	 * Gets the current value of base, or a specific source.
	 *
	 * Note this is not the final value; see Value().
	 */
	NumberType Get(const TOptional<SourceType>& Source = {})
	{
		if (Source.IsSet())
		{
			return Buckets.Contains(Source.GetValue()) ? Buckets[Source.GetValue()] : 0;
		}

		return Base;
	}

	/**
	 * Returns each modification source mapped to the amount attributed to that source.
	 */
	const TMap<SourceType, NumberType>& GetSources() const { return Buckets; }

	/**
	 * Sets the base or source to the given value, overriding any previous value.
	 *
	 * Returns true if the new value differs from the previous.
	 */
	bool Set(const NumberType N, const TOptional<SourceType>& Source = {})
	{
		if (!Source.IsSet())
		{
			if (Base == N)
			{
				return false;
			}

			return true;
		}

		auto ValueToSet = N;
		if (BucketConfig.Contains(Source.GetValue()))
		{
			ValueToSet = Clamp(N, BucketConfig[Source.GetValue()].Min, BucketConfig[Source.GetValue()].Max);
		}

		if (!Buckets.Contains(Source.GetValue()))
		{
			Buckets.Add(Source.GetValue(), 0);
		}

		if (Buckets[Source.GetValue()] == ValueToSet)
		{
			return false;
		}

		Buckets.Add(Source.GetValue(), ValueToSet);
		
		return true;
	}

	/**
	 * Applies a change to the stat by the provided value, optionally bucketed to Source.
	 *
	 * Note this will add to existing sources, rather than override them.
	 */
	void Delta(const NumberType N, const TOptional<SourceType>& Source = {})
	{
		if (!Source.IsSet())
		{
			Set(N + Base);
			return;
		}

		Set(N + Get(Source), Source);
	}

	/**
	 * Retrieves the current total value for the stat.
	 */
	NumberType Value() const
	{
		NumberType Out = Base;

		for (const auto& Entry : Buckets)
		{
			Out += Entry.Value;
		}

		return Clamp(Out, ClampMin, ClampMax);
	}

	/**
	 * True if this stat's overall value is zero.
	 */
	NumberType IsZero() const
	{
		return Value() == 0;
	}

private:
	bool bIsValid = false;

	NumberType Base = 0;
	TMap<SourceType, NumberType> Buckets;

	TOptional<NumberType> ClampMin = {};
	TOptional<NumberType> ClampMax = {};

	struct FBucketConfig
	{
		TOptional<NumberType> Min;
		TOptional<NumberType> Max;
	};

	NumberType Clamp(NumberType Value, const TOptional<NumberType>& Min, const TOptional<NumberType>& Max) const
	{
		if (Min.IsSet() && Value < Min.GetValue())
		{
			Value = Min.GetValue();
		}

		if (Max.IsSet() && Value > Max.GetValue())
		{
			Value = Max.GetValue();
		}

		return Value;
	}

	/**
	 * Fine-grained control of how buckets behave.
	 */
	TMap<SourceType, FBucketConfig> BucketConfig;
};
