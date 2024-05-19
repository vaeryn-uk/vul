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

	/**
	 * Modifies the current value of this measure, returning true if we are not at min (e.g. not dead).
	 */
	bool Modify(const NumberType Delta)
	{
		ModifyCurrent(Delta);

		return CurrentValue() > 0;
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
		Current.Get().Reset();
	}

	float Percent() const
	{
		return static_cast<float>(CurrentValue()) / static_cast<float>(MaxValue());
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