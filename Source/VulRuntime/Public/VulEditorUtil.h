#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * Utility functions available in the editor.
 *
 * Note this is not in our VulEditor module to avoid circ dependencies on the modules;
 * VulRuntime should not include VulEditor.
 * TODO: Either:
 *   1) Merge VulEditor & VulRuntime as the module delineation doesn't make sense, or
 *   2) Move the VulRuntime usage from within VulEditor in to VulEditor so that VulEditor
 *      does not include VulRuntime.
 */
class FVulEditorUtil
{
public:
	/**
	* Presents some in-editor output to the user in a consistent manner.
	*/
	VULRUNTIME_API static void Output(
		const FText& Title,
		const FText& Message,
		const EAppMsgCategory& Category,
		bool ShowDialog = true,
		UObject* Details = nullptr);

	VULRUNTIME_API static void Output(
		const FString& Title,
		const FString& Message,
		const EAppMsgCategory& Category,
		bool ShowDialog = true,
		UObject* Details = nullptr);
};

#endif
