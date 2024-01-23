#include "DataTable/VulDataRepository.h"

TObjectPtr<UScriptStruct> UVulDataRepository::StructType(const FName& TableName) const
{
	return DataTables.FindChecked(TableName)->RowStruct;
}

FVulDataPtr UVulDataRepository::FindPtrChecked(const FName& TableName, const FName& RowName)
{
	return FVulDataPtr(
		TSoftObjectPtr<UVulDataRepository>(this),
		TableName,
		RowName,
		FindRaw<FTableRowBase>(TableName, RowName)
	);
}

bool UVulDataRepository::IsPtrType(const FProperty* Property) const
{
	return Property->GetCPPType() == FVulDataPtr::StaticStruct()->GetStructCPPName();
}
