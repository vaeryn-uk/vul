#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

namespace VulRuntime::Field
{
	/**
	 * Returns true if we consider the given value empty.
	 *
	 * Empty if any of the following are true.
	 *   - is an invalid JSON value
	 *   - is null
	 *   - is an empty string
	 *   - is an array of length 0, or all elements in the array empty (checked recursively).
	 *   - is an empty object, or all values in the object are empty (checked recursively).
	 */
	VULRUNTIME_API bool IsEmpty(const TSharedPtr<FJsonValue>& Value);
}