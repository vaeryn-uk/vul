#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * Utility functions for our editor module.
 */
class FVulEditorUtil
{
public:
	/**
	* Presents some in-editor output to the user in a consistent manner.
	*/
	static void Output(
		const FText& Title,
		const FText& Message,
		const EAppMsgCategory& Category,
		bool ShowDialog = true,
		UObject* Details = nullptr);
};
