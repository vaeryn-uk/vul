#include "DataTable/VulDataPtr.h"
#include "DataTable/VulDataRepository.h"
#include "Field/VulFieldSet.h"

FVulDataPtr::FVulDataPtr(
	UVulDataRepository* InRepository,
	const FName& InTableName,
	const FName& InRowName,
	const void* Data
) : Repository(InRepository), TableName(InTableName), RowName(InRowName), Ptr(Data) {}

bool FVulDataPtr::IsSet() const
{
	return !RowName.IsNone();
}

FVulFieldSet FVulDataPtr::VulFieldSet() const
{
	FVulFieldSet Set;
	
	Set.Add(FVulField::Create(&RowName), "row");
	Set.Add(FVulField::Create(&TableName), "table");
	Set.Add(FVulField::Create(&Repository), "repository");
	
	return Set;
}

FString FVulDataPtr::ToString() const
{
	if (!IsSet())
	{
		return "";
	}
	
	return FString::Printf(TEXT("%s:%s"), *Repository.ToString(), *RowName.ToString());
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
	const auto ResolvedRepository = Repository.LoadSynchronous();
	checkf(::IsValid(ResolvedRepository) && !TableName.IsNone(), TEXT("attempt to resolve struct type for invalid FVulDataPtr"))
	return ResolvedRepository->StructType(TableName);
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

	const auto ResolvedRepository = Repository.LoadSynchronous();
	checkf(::IsValid(ResolvedRepository), TEXT("Failed to resolve repository for row: %s"), *RowName.ToString())

	Ptr = ResolvedRepository->FindRawChecked<FTableRowBase>(TableName, RowName);
	checkf(Ptr != nullptr, TEXT("Failed to load row: %s"), *RowName.ToString())

	return Ptr;
}
