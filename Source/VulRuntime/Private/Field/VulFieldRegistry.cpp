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

TOptional<FVulFieldRegistry::FEntry> FVulFieldRegistry::GetBaseType(const FString& TypeId) const
{
	const auto Type = GetType(TypeId);
	if (!Type.IsSet())
	{
		return {};
	}

	for (const auto Entry : Entries)
	{
		if (Type->BaseType == Entry.Value.TypeId)
		{
			return Entry.Value;
		}
	}

	return {};
}
