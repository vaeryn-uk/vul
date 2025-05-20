#include "Field/VulFieldRegistry.h"

TArray<FVulFieldRegistry::FEntry> FVulFieldRegistry::GetSubtypes(const FString& TypeId) const
{
	TArray<FEntry> Out;

	for (const auto Entry : Entries)
	{
		if (Entry.Value.BaseType == TypeId)
		{
			Out.Add(Entry.Value);
		}
	}

	return Out;
}
