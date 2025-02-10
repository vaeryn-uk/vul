#include "DataTable/VulDataPtr.h"
#include "DataTable/VulDataRepository.h"

bool FVulDataPtr::IsSet() const
{
	return !RowName.IsNone();
}

FString FVulDataPtr::ToString() const
{
	if (!IsSet())
	{
		return "";
	}
	
	return FString::Printf(TEXT("%s:%s"), *Repository->GetName(), *RowName.ToString());
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
	return Repository->StructType(TableName);
}

bool FVulDataPtr::IsPendingInitialization() const
{
	return IsSet() && TableName.IsNone();
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

	Ptr = Repository->FindRawChecked<FTableRowBase>(TableName, RowName);
	checkf(Ptr != nullptr, TEXT("Failed to load row: %s"), *RowName.ToString())

	return Ptr;
}
