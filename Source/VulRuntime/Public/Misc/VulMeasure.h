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
	TVulMeasure() = default;
	explicit TVulMeasure(const NumberType InMax) : Current(InMax), Max(InMax) {};
	explicit TVulMeasure(const NumberType InCurrent, const NumberType InMax) : Current(InCurrent), Max(InMax) {};

	/**
	 * Modifies the current value of this measure, returning true if we are not at min (e.g. not dead).
	 */
	bool Modify(const NumberType Delta)
	{
		ModifyCurrent(Delta);

		return Current.Current() > 0;
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
		return Current.Current() >= Amount;
	}

	/**
	 * Clears this measure so its current value is empty/zero.
	 */
	void Empty()
	{
		Current.Reset();
	}

	float Percent() const
	{
		return static_cast<float>(Current.Current()) / static_cast<float>(Max.Current());
	}

	NumberType CurrentValue() const
	{
		return Current.Current();
	}

	NumberType MaxValue() const
	{
		return Max.Current();
	}

	const TVulNumber<NumberType>& GetCurrent() const
	{
		return Current;
	}

	const TVulNumber<NumberType>& GetMax() const
	{
		return Max;
	}

private:
	void ModifyCurrent(const NumberType Amount)
	{
		Current.ModifyBase(Amount, {{0, Max.Current()}});
	}

	TVulNumber<NumberType> Current = 0;
	TVulNumber<NumberType> Max = 0;
};