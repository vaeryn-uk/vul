#include "VulRuntimeSettings.h"

bool UVulRuntimeSettings::IsTooltipEnabled() const
{
	return !TooltipWidget.IsNull();
}

const FVulRichTextIconDefinition* UVulRuntimeSettings::ResolveIcon(const FName& RowName) const
{
	if (!ensureMsgf(!IconSet.IsNull(), TEXT("VulRuntime Settings: IconMap not set. Cannot resolve icon data")))
	{
		return nullptr;
	}

	return IconSet.LoadSynchronous()->FindRow<FVulRichTextIconDefinition>(
		RowName,
		"UVulRuntimeSettings::ResolveIcon",
		false // We support placeholder icons if row is not set, so don't warn on this as it clogs logs.
	);
}

const UVulRuntimeSettings* VulRuntime::Settings()
{
	return GetDefault<UVulRuntimeSettings>();
}
