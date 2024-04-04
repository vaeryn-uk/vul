#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * Logic where something can be toggled on from multiple contexts.
 *
 * Designed for UI code, where multiple elements might trigger some state (e.g. a tooltip
 * being shown), and that thing should remain triggered until all contexts have disabled it.
 */
template <class ContextType = FString>
class TVulContextToggle
{
public:
	TVulContextToggle() = default;

	/**
	 * Toggle on for a given context.
	 *
	 * Returns true if this changes the overall enabled state of the toggle.
	 */
	bool Enable(const ContextType& Context)
	{
		const auto Previous = IsEnabled();
		Contexts.Add(Context);
		return Previous != IsEnabled();
	}

	/**
	 * Toggle on for a given context.
	 *
	 * Returns true if this changes the overall enabled state of the toggle.
	 */
	bool Disable(const ContextType& Context)
	{
		const auto Previous = IsEnabled();
		Contexts.Remove(Context);
		return Previous != IsEnabled();
	}

	/**
	 * True if this toggle is enabled. That is, it is enabled from any single context.
	 */
	bool IsEnabled() const
	{
		return !Contexts.IsEmpty();
	}

	/**
	 * True if this toggle is enabled for the given context.
	 */
	bool IsEnabled(const ContextType& Context) const
	{
		return Contexts.Contains(Context);
	}

	/**
	 * Resets the toggle, forcibly disabling all contexts.
	 */
	void Reset()
	{
		Contexts.Reset();
	}

private:
	TSet<ContextType> Contexts = {};
};

/**
 * A common toggle where the contexts are strings. Callers must use the same string for enable & disable
 * from the same context.
 */
typedef TVulContextToggle<> TVulStrCtxToggle;