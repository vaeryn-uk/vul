#include "DataTable/VulDataPtr.h"
#include "DataTable/VulDataRepository.h"

bool FVulDataPtr::IsSet() const
{
	return !RowName.IsNone();
}

const FName& FVulDataPtr::GetRowName() const
{
	return RowName;
}

const FName& FVulDataPtr::GetTableName() const
{
	return TableName;
}

TObjectPtr<UScriptStruct> FVulDataPtr::StructType() const
{
	checkf(IsValid(), TEXT("attempt to resolve struct type for invalid FVulDataPtr"))
	return Repository.LoadSynchronous()->StructType(TableName);
}

bool FVulDataPtr::IsPendingInitialization() const
{
	return IsSet() && (Repository.IsNull() || TableName.IsNone());
}

bool FVulDataPtr::IsValid() const
{
	return IsSet() && !IsPendingInitialization();
}

const void* FVulDataPtr::EnsurePtr() const
{
	checkf(IsValid(), TEXT("Attempt to load ptr for invalid FVulDataPtr"))

	if (Ptr != nullptr)
	{
		return Ptr;
	}

	Ptr = Repository.LoadSynchronous()->FindRaw<FTableRowBase>(TableName, RowName);
	checkf(Ptr != nullptr, TEXT("Failed to load row: %s"), *RowName.ToString())

	return Ptr;
}
