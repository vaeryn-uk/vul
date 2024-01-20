#include "DataTable/VulDataRepository.h"

bool FVulDataRef::IsInitialized() const
{
	return IsValid(Repository) && !TableName.IsNone();
}

bool UVulDataRepository::IsRefType(const FProperty* Property) const
{
	return Property->GetCPPType() == FVulDataRef::StaticStruct()->GetStructCPPName();
}
