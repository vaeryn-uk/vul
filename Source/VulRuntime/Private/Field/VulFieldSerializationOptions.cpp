#include "Field/VulFieldSerializationOptions.h"

TMap<FString, bool> FVulFieldSerializationFlags::GlobalDefaults = {
	{VulFieldSerializationFlag_Referencing, true},
	{VulFieldSerializationFlag_AssetReferencing, true},
};

bool FVulFieldSerializationFlags::IsEnabled(const FString& Option) const
{
	return Resolve(Option);
}

bool FVulFieldSerializationFlags::Resolve(const FString& Option) const
{
	if (Flags.Contains(Option))
	{
		return Flags[Option];
	}

	if (GlobalDefaults.Contains(Option))
	{
		return GlobalDefaults[Option];
	}

	return false;
}
