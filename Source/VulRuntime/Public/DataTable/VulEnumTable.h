#pragma once

#include "CoreMinimal.h"
#include "Misc/VulEnum.h"
#include "UObject/Object.h"

/**
 * Access to a data table which houses rows that are accessed by an Enum.
 *
 * Each row in the data table corresponds to one value of EnumType. This is useful
 * tool to blend config, data-driven solutions with in-code logic for complex scenarios
 * where building functionality in to config is not worth the effort. 
 *
 * For example, you may have a data table of spells, but one spell behaves in a very
 * specific way. To implement this, you can define your spells in a data table, containing
 * standard properties such as damage and range - these are implemented generically.
 * You can then create an enum in your code to access these rows and implement your specific
 * functionality if the row is for your given spell. The enum yields an explicit, concrete
 * binding between your config and its uses in code.
 */
template <typename RowType, typename EnumType, typename RowPtrType = RowType*, typename ConstRowPtrType = const RowType*>
class TVulEnumTable
{
public:
	TVulEnumTable() = default;
	virtual ~TVulEnumTable() = default;

	ConstRowPtrType Load(const EnumType Value) const
	{
		LoadRows();
		return Definitions.Contains(Value) ? Definitions[Value] : nullptr;
	}

	ConstRowPtrType Load(const FName& RowName) const
	{
		LoadRows();
		return ByRow.Contains(RowName) ? ByRow[RowName] : nullptr;
	}

	TArray<ConstRowPtrType> LoadAll() const
	{
		return LoadAll([](ConstRowPtrType Row) { return true; });
	}

	template <typename PredicateType>
	TArray<ConstRowPtrType> LoadAll(PredicateType Predicate) const
	{
		LoadRows();
		
		TArray<ConstRowPtrType> Out;

		for (const auto& Entry : ByRow)
		{
			if (Predicate(Entry.Value))
			{
				Out.Add(Entry.Value);
			}
		}
		
		return Out;
	}

	/**
	 * Ensures this table contains a row for each expected enum value, returning an empty array if ok.
	 *
	 * Returns a list of enum values that we cannot find rows for.
	 *
	 * Optionally exclude some enum values from the validation. Useful for a "none" enum type.
	 */
	TArray<EnumType> ValidateEnums(const TArray<EnumType>& Exclude = {}) const
	{
		TArray<EnumType> Missing;

		for (const auto Val : VulRuntime::Enum::Values<EnumType>())
		{
			if (Exclude.Contains(Val))
			{
				continue;
			}
			
			if (Load(Val) == nullptr)
			{
				Missing.Add(Val);
			}
		}

		return Missing;
	}

protected:
	/**
	 * Loads and returns the underlying UE data table.
	 *
	 * Commonly this involves loading an object pointer from project config.
	 *
	 * This will be cached once loaded via a weak ptr, so the asset must be kept in
	 * memory by your implementation.
	 */
	virtual UDataTable* LoadTable() const = 0;

	/**
	 * Given a data table row, return the enum value.
	 *
	 * Commonly this is a known property on the row.
	 */
	virtual EnumType GetEnumValue(ConstRowPtrType Row) const = 0;

	/**
	 * Given a data table row, return the RowName value.
	 *
	 * All data table rows in UE have a RowName; using UVulDataTableSource::RowNameMetaSpecifier will
	 * automatically copy a RowName to a property within the row.
	 */
	virtual FName GetRowName(ConstRowPtrType Row) const = 0;

	UDataTable* GetTable() const
	{
		if (!Table.IsValid())
		{
			Table = LoadTable();
		}

		return Table.Get();
	}

	/**
	 * For concrete implementations: implement a function which returns a subset of rows
	 * and have it cached indefinitely.
	 */
	template <typename PredicateType>
	TArray<ConstRowPtrType>& Filter(const FString& Key, PredicateType Predicate) const
	{
		if (FilteredCache.Contains(Key))
		{
			return FilteredCache[Key];
		}

		FilteredCache.Add(Key, LoadAll().FilterByPredicate(Predicate));

		return FilteredCache[Key];
	}

	/**
	 * Actually loads all rows.
	 *
	 * Must use StoreRow() for each row that is loaded.
	 */
	virtual void DoLoadRows() const = 0;
	
	mutable TMap<EnumType, RowPtrType> Definitions;
	mutable TMap<FName, RowPtrType> ByRow;
private:
	mutable TMap<FString, TArray<ConstRowPtrType>> FilteredCache;
	
	void LoadRows() const
	{
		if (!Loaded)
		{
			DoLoadRows();
			Loaded = true;
		}
	}

	mutable TWeakObjectPtr<UDataTable> Table;
	mutable bool Loaded = false;
};

/**
 * A TVulEnumTable that simply wraps a data table ptr and provides a simpler interface to implement.
 *
 * Note as this is not a UObject, this does not ensure the lifetime of the data table. We store
 * a TWeakObjectPtr and surface an IsValid function to ensure that the table can be used safely.
 */
template <typename RowType, typename EnumType>
class TVulEnumDataTable : public TVulEnumTable<RowType, EnumType>
{
public:
	TVulEnumDataTable() = default;
	
	void SetDataTable(const TWeakObjectPtr<UDataTable>& InDataTable)
	{
		DataTable = InDataTable;
	}

	bool IsValid() const { return DataTable.IsValid(); }
protected:
	virtual UDataTable* LoadTable() const override { return DataTable.Get(); }
	virtual EnumType GetEnumValue(const RowType* Row) const override { return GetRowNameAndEnum(Row).Value; }
	virtual FName GetRowName(const RowType* Row) const override { return GetRowNameAndEnum(Row).Key; }

	virtual void DoLoadRows() const override
	{
		if (!ensureAlwaysMsgf(DataTable.IsValid(), TEXT("TVulEnumTable: must provide a data table")))
		{
			return;
		}

		TArray<RowType*> Rows;
		DataTable->GetAllRows("TVulEnumTable", Rows);

		for (const auto Row : Rows)
		{
			this->Definitions.Add(this->GetEnumValue(Row), Row);
			this->ByRow.Add(this->GetRowName(Row), Row);
		}
	}

	/**
	 * Returns a row's RowName (FName) and enum Value (EnumType) in a single method.
	 *
	 * Useful when both are simply properties on the row.
	 */
	virtual TTuple<FName, EnumType> GetRowNameAndEnum(const RowType* Row) const = 0;

private:
	TWeakObjectPtr<UDataTable> DataTable = nullptr;
};