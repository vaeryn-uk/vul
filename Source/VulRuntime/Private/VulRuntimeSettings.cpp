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
		"UVulRuntimeSettings::ResolveIcon"
	);
}

const UVulRuntimeSettings* VulRuntime::Settings()
{
	return GetDefault<UVulRuntimeSettings>();
}
