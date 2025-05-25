#pragma once

#include "CoreMinimal.h"
#include "VulNumber.h"
#include "UObject/Object.h"

/**
 * A measure is an amount of some resource, between 0 and some fixed max. Example: HP.
 *
 * Template parameter must be a numeric type that supports standard arithmetic operators.
 */
template <typename NumberType>
struct TVulMeasure
{
	VULFLD_TYPE(TVulMeasure, "VulMeasure")
	
	TVulMeasure()
	{
		Init(0, 0);
	}

	explicit TVulMeasure(const NumberType InMax)
	{
		Init(InMax, InMax);
	}

	explicit TVulMeasure(const NumberType InCurrent, const NumberType InMax)
	{
		Init(InCurrent, InMax);
	}

	TVulMeasure(const TVulMeasure& Other)
	{
		Current = MakeShared<TVulNumber<NumberType>>(*Other.Current);
		Max = MakeShared<TVulNumber<NumberType>>(*Other.Max);
	}

	TVulMeasure& operator=(const TVulMeasure& Other)
	{
		if (this != &Other) // Guard against self-assignment
		{
			*Current = *Other.Current;
			*Max = *Other.Max;
		}
		
		return *this;
	}

	/**
	 * Creates a snapshot of the current measure with all modifications removed.
	 *
	 * The result can be freely changed without impacting this measure. Useful for
	 * performing changes to a measure to query what will happen, e.g. "if 10 damage is done,
	 * is a character dead?"
	 */
	TVulMeasure Snapshot() const
	{
		return TVulMeasure(CurrentValue(), MaxValue());
	}

	static TVulMeasure Sum(const TArray<TVulMeasure>& Measures)
	{
		NumberType Sum = 0;
		NumberType Max = 0;

		for (const auto& Measure : Measures)
		{
			Sum += Measure.CurrentValue();
			Max += Measure.MaxValue();
		}

		return TVulMeasure(Sum, Max);
	}

	/**
	 * Modifies the current value of this measure, returning true if we are not at min (e.g. not dead).
	 */
	bool Modify(const NumberType Delta)
	{
		ModifyCurrent(Delta);

		return CurrentValue() > 0;
	}

	/**
	 * Modifies the current value of this measure, returning true if there was a change in the current value.
	 */
	bool Change(const NumberType Delta)
	{
		const auto Before = CurrentValue();

		ModifyCurrent(Delta);

		return Before != CurrentValue();
	}

	/**
	 * Sets the current value to the new amount, returning true if the value changed.
	 *
	 * Normal measure constraints apply to this change (e.g. the resulting current will not be lower than 0).
	 */
	bool SetCurrent(const NumberType NewVal)
	{
		return Change(NewVal - CurrentValue());
	}

	/**
	 * How far away is this measure away from its max value?
	 */
	NumberType Missing() const
	{
		return Max->Value() - Current->Value();
	}

	/**
	 * Deducts Amount from this resource, only if we have enough of the resource.
	 *
	 * Returns true if we consumed, or false if we did not have enough.
	 */
	bool Consume(const NumberType Amount)
	{
		if (!CanConsume(Amount))
		{
			return false;
		}

		ModifyCurrent(-Amount);
		return true;
	}

	/**
	 * Returns true we have the requested amount of energy, i.e. @see Consume will succeed.
	 */
	bool CanConsume(const NumberType Amount) const
	{
		return CurrentValue() >= Amount;
	}

	/**
	 * Clears this measure so its current value is empty/zero.
	 */
	void Empty()
	{
		Current.Get()->Reset();
	}

	/**
	 * Returns true if this measure is at its max value.
	 */
	bool IsFull() const
	{
		return Percent() >= 1.f;
	}

	float Percent() const
	{
		return static_cast<float>(CurrentValue()) / static_cast<float>(MaxValue());
	}

	FVulFieldSet VulFieldSet() const
	{
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Current), "current");
		Set.Add(FVulField::Create(&Max), "max");
		return Set;
	}

	NumberType CurrentValue() const
	{
		return GetCurrent().Value();
	}

	NumberType MaxValue() const
	{
		return GetMax().Value();
	}

	const TVulNumber<NumberType>& GetCurrent() const
	{
		return *Current;
	}

	const TVulNumber<NumberType>& GetMax() const
	{
		return *Max;
	}

private:
	void Init(const NumberType InCurrent, const NumberType InMax)
	{
		Max = MakeShared<TVulNumber<NumberType>>(InMax);
		auto Zero = MakeShared<TVulNumber<NumberType>>(0);
		Current = MakeShared<TVulNumber<NumberType>>(InCurrent, TVulNumber<NumberType>::FClamp({Zero, Max}));
	}

	void ModifyCurrent(const NumberType Amount)
	{
		Current.Get()->ModifyBase(Amount);
	}

	TSharedPtr<TVulNumber<NumberType>> Current;
	TSharedPtr<TVulNumber<NumberType>> Max;
};