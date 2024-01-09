#include "DataTable/VulDataRepository.h"

bool UVulDataRepository::IsRefType(const FProperty* Property) const
{
	return Property->GetCPPType() == FVulDataRef::StaticStruct()->GetStructCPPName();
}
