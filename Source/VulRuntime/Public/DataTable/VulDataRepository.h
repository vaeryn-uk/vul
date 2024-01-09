#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulDataRepository.generated.h"

/**
 * A reference to a row in a data table. This can be embedded in USTRUCTs and deferenced
 * as required to get the referenced row.
 *
 * You may use a ref in a USTRUCT property, array value or map value. You must specify
 * a VulDataTable metadata specified, e.g.:
 *
 *   UPROPERTY(meta=(VulDataTable="MyOtherTable"))
 *   FVulDataRef SomeOtherRow;
 *
 * Where "MyOtherTable" is the name given to a data table in a UVulDataRepository.
 *
 * You must instantiate rows via a UVulDataRepository for refs to function correctly.
 *
 * For support in the editor, this class is non-templated, but does accept a when actually
 * fetching the referenced row.
 */
USTRUCT()
struct VULRUNTIME_API FVulDataRef
{
	GENERATED_BODY()

	FVulDataRef() = default;
	FVulDataRef(const FName& InRowName) : RowName(InRowName) {};

	/**
	 * The RowName of the row we're referencing.
	 */
	UPROPERTY(EditAnywhere)
	FName RowName;

	/**
	 * Fetches the referenced row, encountering an error if the row does not exist
	 * or this reference has not been initialized correctly.
	 */
	template <typename RowType>
	RowType* FindChecked() const;

	friend class UVulDataRepository;

private:
	UPROPERTY()
	class UVulDataRepository* Repository = nullptr;

	FName TableName;
};

/**
 * A data repository provides access to one or more data tables that may have references
 * between their rows.
 */
UCLASS()
class VULRUNTIME_API UVulDataRepository : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Finds a row reference, ensuring the row exists.
	 */
	template <typename RowType>
	RowType* FindChecked(const FName& TableName, const FName& RowName);

	/**
	 * The data tables that make up this repository. Each table is indexed by a user-defined name
	 * which is how a table is identified in ref UPROPERTY metadata specifiers.
	 *
	 * Note that this is a live pointer to keep all data tables in memory always.
	 */
	UPROPERTY(EditAnywhere)
	TMap<FName, UDataTable*> DataTables;

private:
	void InitStruct(const UDataTable* Table, UScriptStruct* Struct, void* Data);

	bool IsRefType(const FProperty* Property) const;

	void InitRefProperty(const FProperty* Property, FVulDataRef* Ref, const UScriptStruct* Struct)
	{
		if (Ref->Repository != nullptr)
		{
			// Already initialized.
			return;
		}

		checkf(
			Property->HasMetaData(TEXT("VulDataTable")),
			TEXT("%s: meta field VulDataTable must be specified on FVulDataRef properties"),
			*Struct->GetStructCPPName()
		);

		const auto RefTable = FName(Property->GetMetaData(FName(TEXT("VulDataTable"))));
		checkf(DataTables.Contains(RefTable), TEXT("Data repository does not have table %s"), RefTable);

		Ref->Repository = this;
		Ref->TableName = RefTable;
	}
};

template <typename RowType>
RowType* FVulDataRef::FindChecked() const
{
	checkf(Repository != nullptr, TEXT("Repository not set"));
	return Repository->FindChecked<RowType>(TableName, RowName);
}

template <typename RowType>
RowType* UVulDataRepository::FindChecked(const FName& TableName, const FName& RowName)
{
	auto Table = DataTables.FindChecked(TableName);
	auto Row = Table->FindRow<RowType>(RowName, TEXT("VulDataRepository FindChecked"));
	checkf(Row != nullptr, TEXT("Cannot find row %s in table %s"), *TableName.ToString(), *RowName.ToString());

	InitStruct(Table, Table->RowStruct, Row);

	return Row;
}

inline void UVulDataRepository::InitStruct(const UDataTable* Table, UScriptStruct* Struct, void* Data)
{
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		if (IsRefType(*It))
		{
			auto RefProperty = reinterpret_cast<FVulDataRef*>(It->ContainerPtrToValuePtr<void>(Data));
			InitRefProperty(*It, RefProperty, Struct);
		} else if (const auto ArrayProperty = CastField<FArrayProperty>(*It))
		{
			FScriptArrayHelper Helper(ArrayProperty, It->ContainerPtrToValuePtr<void>(Data));

			for (int i = 0; i < Helper.Num(); ++i)
			{
				auto InnerData = Helper.GetElementPtr(i);

				if (IsRefType(ArrayProperty->Inner))
				{
					InitRefProperty(ArrayProperty, reinterpret_cast<FVulDataRef*>(InnerData), Struct);
				} else if (const auto StructProp = CastField<FStructProperty>(ArrayProperty->Inner))
				{
					InitStruct(Table, StructProp->Struct, InnerData);
				}
			}
		} else if (const auto MapProperty = CastField<FMapProperty>(*It))
		{
			FScriptMapHelper Helper(MapProperty, It->ContainerPtrToValuePtr<void>(Data));

			for (int i = 0; i < Helper.Num(); ++i)
			{
				// TODO: Support for refs as keys?
				auto ValueData = Helper.GetValuePtr(i);

				if (IsRefType(MapProperty->KeyProp))
				{
					InitRefProperty(MapProperty, reinterpret_cast<FVulDataRef*>(ValueData), Struct);
				} else if (auto ValueProp = CastField<FStructProperty>(MapProperty->ValueProp))
				{
					InitStruct(Table, ValueProp->Struct, ValueData);
				}
			}
		}
	}
}