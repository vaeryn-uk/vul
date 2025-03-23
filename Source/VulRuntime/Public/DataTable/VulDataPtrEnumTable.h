#pragma once

#include "CoreMinimal.h"
#include "VulEnumTable.h"
#include "UObject/Object.h"
#include "VulDataRepository.h"

/**
 * For when your enums are managed through a UVulDataRepository.
 */
template <typename RowType, typename EnumType>
class TVulDataPtrEnumTable
	: public TVulEnumTable<TVulDataPtr<RowType>, EnumType, TVulDataPtr<RowType>, const TVulDataPtr<RowType>>
{
public:
	TVulDataPtrEnumTable() = default;
	
	void SetRepo(UVulDataRepository* InRepo, const FName& InTableName)
	{
		Repo = InRepo;
		TableName = InTableName;
	}

	bool IsValid() const
	{
		return Repo.IsValid() && !TableName.IsNone();
	}

protected:
	virtual UDataTable* LoadTable() const override
	{
		checkf(
			Repo->DataTables.Contains(TableName),
			TEXT("Cannot LoadTable() for UVulDataRepository no table called %s"),
			*TableName.ToString()
		)
		
		return Repo->DataTables[TableName];
	}

	virtual void DoLoadRows() const override
	{
		for (const auto Row : Repo->LoadAllPtrs<RowType>(TableName))
		{
			this->Definitions.Add(this->GetEnumValue(Row), Row);
			this->ByRow.Add(this->GetRowName(Row), Row);
		}
	}

private:
	TWeakObjectPtr<UVulDataRepository> Repo;
	FName TableName;
};