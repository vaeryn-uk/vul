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

	// TODO: Caching this Ptr causes issues when garbage collecting. It looks like
	// the underlying row data is disappearing so we can't just keep a raw pointer
	// like this. Disabling this fixes the issue, but means we'll be re-initing
	// the struct every time we need it.
	// if (Ptr != nullptr)
	// {
	// 	return Ptr;
	// }

	Ptr = Repository.LoadSynchronous()->FindRaw<FTableRowBase>(TableName, RowName);
	checkf(Ptr != nullptr, TEXT("Failed to load row: %s"), *RowName.ToString())

	return Ptr;
}
