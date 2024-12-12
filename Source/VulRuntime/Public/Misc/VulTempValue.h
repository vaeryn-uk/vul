#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UObject/Object.h"

/**
 * A temp value can be used to store a short-lived override of a value, where
 * we want to shortly restore the previous value.
 *
 * This is a simple wrapper around a stack, thus will return from Restore() in
 * the LIFO order things were saved via Store().
 *
 * Use this when you want to change a property of an actor temporarily, then return
 * to whatever value was present before.
 */
template <typename Value>
struct TVulTempValue
{
	void Store(const Value Old)
	{
		Stack.Push(Old);
	}
	
	TOptional<Value> Restore()
	{
		if (Stack.IsEmpty())
		{
			Value V{};
			return V;
		}

		return Stack.Pop();
	}
	
private:
	TArray<Value> Stack = {};
};

/**
 * Application of a temporary visibility to a widget that can be conveniently
 * restored later.
 */
struct FVulTempWidgetVisibility
{
	/**
	 * Set the widget visibility temporarily to the given value. 
	 */
	void Store(UWidget* Widget, const ESlateVisibility Visibility)
	{
		if (IsValid(Widget))
		{
			TempValue.Store(Visibility);
			Widget->SetVisibility(Visibility);
		}
	}

	/**
	 * Restore the visibility of the widget that was set when we previously called Store.
	 */
	void Restore(UWidget* Widget)
	{
		const auto Ret = TempValue.Restore();
		
		if (IsValid(Widget) && Ret.IsSet())
		{
			Widget->SetVisibility(Ret.GetValue());
		}
	}
	
private:
	TVulTempValue<ESlateVisibility> TempValue;
};
