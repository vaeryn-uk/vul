#pragma once

#include "CoreMinimal.h"
#include "VulRuntimeSettings.h"
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
 *
 * To use, extend this class with concrete types and implement the required functions.
 * You will need to define your row type which includes the appropriate enum property.
 */
template <typename RowType, typename EnumType>
class TVulEnumTable
{
public:
	TVulEnumTable() = default;
	virtual ~TVulEnumTable() = default;

	RowType* Load(const EnumType Value) const
	{
		LoadRows();
		return Definitions.Find(Value);
	}

	RowType* Load(const FName& RowName) const
	{
		LoadRows();
		return ByRow.Find(RowName);
	}

	TArray<RowType*> LoadAll() const
	{
		return LoadAll([](RowType* Row) { return true; });
	}

	template <typename PredicateType>
	TArray<RowType*> LoadAll(PredicateType Predicate) const
	{
		LoadRows();
		
		TArray<RowType*> Out;

		for (auto& Entry : ByRow)
		{
			if (Predicate(&Entry.Value))
			{
				Out.Add(&Entry.Value);
			}
		}
		
		return Out;
	}

	RowType LoadChecked(const EnumType Effect) const
	{
		const auto Found = Load(Effect);
		checkf(Found != nullptr, TEXT("Unknown enum value"));
		return *Found;
	}

	/**
	 * Ensures this table contains a row for each expected enum value, returning an empty array if ok.
	 *
	 * Returns a list of enum values that we cannot find rows for.
	 */
	TArray<EnumType> ValidateEnums() const
	{
		TArray<EnumType> Missing;

		for (const auto Val : VulRuntime::Enum::Values<EnumType>())
		{
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
	virtual EnumType GetEnumValue(const RowType* Row) const = 0;

	/**
	 * Given a data table row, return the RowName value.
	 *
	 * All data table rows in UE have a RowName; using UVulDataTableSource::RowNameMetaSpecifier will
	 * automatically copy a RowName to a property within the row.
	 */
	virtual FName GetRowName(const RowType* Row) const = 0;

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
	const TArray<RowType*>& Filter(const FString& Key, PredicateType Predicate) const
	{
		if (FilteredCache.Contains(Key))
		{
			return FilteredCache[Key];
		}

		FilteredCache.Add(Key, LoadAll().FilterByPredicate(Predicate));

		return FilteredCache[Key];
	}

private:
	mutable TMap<FString, TArray<RowType*>> FilteredCache;
	
	void LoadRows() const
	{
		if (!Loaded)
		{
			const auto Dt = GetTable();
			if (!ensureAlwaysMsgf(IsValid(Dt), TEXT("TVulEnumTable: must provide a data table")))
			{
				return;
			}

			TArray<RowType*> Rows;
			Dt->GetAllRows("TVulEnumTable", Rows);

			Definitions = TMap<EnumType, RowType>();

			for (const auto& Row : Rows)
			{
				Definitions.Add(GetEnumValue(Row), *Row);
				ByRow.Add(GetRowName(Row), *Row);
			}

			Loaded = true;
		}
	}

	mutable TWeakObjectPtr<UDataTable> Table;
	mutable bool Loaded = false;
	mutable TMap<EnumType, RowType> Definitions;
	mutable TMap<FName, RowType> ByRow;
};

/**
 * A TVulEnumTable that simply wraps a data table ptr and provides a simpler interface to implement.
 *
 * Note as this is not a UObject, this does not ensure the lifetime of the data table. We store
 * a TWeakObjectPtr and surface an IsValid function to ensure that the table can be used safely.
 */
template <typename RowType, typename EnumType>
class TVulEnumTableWrapped : public TVulEnumTable<RowType, EnumType>
{
public:
	TVulEnumTableWrapped() = default;
	
	void SetDataTable(const TWeakObjectPtr<UDataTable>& InDataTable)
	{
		DataTable = InDataTable;
	}

	bool IsValid() const { return DataTable.IsValid(); }
protected:
	virtual UDataTable* LoadTable() const override { return DataTable.Get(); }
	virtual EnumType GetEnumValue(const RowType* Row) const override { return GetRowNameAndEnum(Row).Value; }
	virtual FName GetRowName(const RowType* Row) const override { return GetRowNameAndEnum(Row).Key; }

	/**
	 * Returns a row's RowName (FName) and enum Value (EnumType) in a single method.
	 *
	 * Useful when both are simply properties on the row.
	 */
	virtual TTuple<FName, EnumType> GetRowNameAndEnum(const RowType* Row) const = 0;

private:
	TWeakObjectPtr<UDataTable> DataTable = nullptr;
};