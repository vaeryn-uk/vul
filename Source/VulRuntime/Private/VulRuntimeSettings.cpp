#include "VulRuntimeSettings.h"

bool UVulRuntimeSettings::IsTooltipEnabled() const
{
	return !TooltipWidget.IsNull();
}

const UVulRuntimeSettings* VulRuntime::Settings()
{
	return GetDefault<UVulRuntimeSettings>();
}
