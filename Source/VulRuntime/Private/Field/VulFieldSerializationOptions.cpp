#include "Field/VulFieldSerializationOptions.h"
#include "Field/VulFieldUtil.h"

TMap<FString, bool> FVulFieldSerializationFlags::GlobalDefaults = {
	{VulFieldSerializationFlag_Referencing, true},
	{VulFieldSerializationFlag_AssetReferencing, true},
};

void FVulFieldSerializationFlags::Set(const FString& Option, const bool Value, const FString& Path)
{
	if (!PathFlags.Contains(Path))
	{
		PathFlags.Add(Path, {});
	}

	PathFlags[Path].Add(Option, Value);
}

bool FVulFieldSerializationFlags::IsEnabled(const FString& Option, const VulRuntime::Field::FPath& Path) const
{
	return Resolve(Option, Path);
}

bool FVulFieldSerializationFlags::Resolve(const FString& Option, const VulRuntime::Field::FPath& Path) const
{
	for (const auto& Entry : PathFlags)
	{
		if (Entry.Key == "")
		{
			continue;
		}
		
		if (Entry.Value.Contains(Option) && VulRuntime::Field::PathMatch(Path, Entry.Key))
		{
			return Entry.Value[Option];
		}
	}

	if (PathFlags.Contains("") && PathFlags[""].Contains(Option))
	{
		return PathFlags[""][Option];
	}
	
	if (GlobalDefaults.Contains(Option))
	{
		return GlobalDefaults[Option];
	}

	return false;
}
