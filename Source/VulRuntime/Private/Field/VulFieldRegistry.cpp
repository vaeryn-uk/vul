#include "Field/VulFieldRegistry.h"

TArray<FVulFieldRegistry::FEntry> FVulFieldRegistry::ConnectedEntries(const FString& TypeId) const
{
	TArray<FEntry> Out;

	FString CurrentId = TypeId;
	while (true)
	{
		for (const auto Entry : Entries)
		{
			if (Entry.Value.BaseType == CurrentId)
			{
				Out.Add(Entry.Value);
			}
		}

		if (Entries.Contains(CurrentId) && Entries[CurrentId].BaseType.IsSet())
		{
			CurrentId = Entries[CurrentId].BaseType.GetValue();
			continue;
		}

		break;
	}

	return Out;
}

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
