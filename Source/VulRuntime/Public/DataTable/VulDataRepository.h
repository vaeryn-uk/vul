#pragma once

#include "CoreMinimal.h"
#include "VulDataPtr.h"
#include "UObject/Object.h"
#include "VulDataRepository.generated.h"

/**
 * A data repository provides access to one or more data tables that may have references
 * between their rows.
 */
UCLASS()
class VULRUNTIME_API UVulDataRepository : public UObject
{
	GENERATED_BODY()
public:

	TObjectPtr<UScriptStruct> StructType(const FName& TableName) const;

	/**
	 * Finds a row by table & row name, asserting the row exists.
	 *
	 * Returned is a TVulDataPtr; a lightweight wrapper that saves copying of
	 * struct data. This can be passed safely around calling code. See FVulDataPtr
	 * for a summary of this ptr type.
	 */
	template <typename RowType>
	TVulDataPtr<RowType> FindChecked(const FName& TableName, const FName& RowName);

	/**
	 * The data tables that make up this repository. Each table is indexed by a user-defined name
	 * which is how a table is identified in ref UPROPERTY metadata specifiers.
	 *
	 * Note that this is a live pointer to keep all data tables in memory always.
	 */
	UPROPERTY(EditAnywhere)
	TMap<FName, UDataTable*> DataTables;

private:
	friend FVulDataPtr;

	template <typename RowType>
	const RowType* FindRaw(const FName& TableName, const FName& RowName);

	FVulDataPtr FindPtrChecked(const FName& TableName, const FName& RowName);

	void InitStruct(const UDataTable* Table, UScriptStruct* Struct, void* Data);

	bool IsPtrType(const FProperty* Property) const;

	void InitPtrProperty(const FProperty* Property, FVulDataPtr* Ptr, const UScriptStruct* Struct)
	{
		if (!Ptr->IsPendingInitialization())
		{
			// Already initialized or a null ptr.
			return;
		}

		checkf(
			Property->HasMetaData(TEXT("VulDataTable")),
			TEXT("%s: meta field VulDataTable must be specified on FVulDataRef properties"),
			*Struct->GetStructCPPName()
		);

		const auto RefTable = FName(Property->GetMetaData(FName(TEXT("VulDataTable"))));
		checkf(DataTables.Contains(RefTable), TEXT("Data repository does not have table %s"), RefTable);

		Ptr->Repository = this;
		Ptr->TableName = RefTable;

		checkf(Ptr->IsValid(), TEXT("InitPtrProperty resulted in an invalid FVulDataPtr"))
	}
};

template <typename RowType>
const RowType* UVulDataRepository::FindRaw(const FName& TableName, const FName& RowName)
{
	auto Table = DataTables.FindChecked(TableName);
	auto Row = Table->FindRow<RowType>(RowName, TEXT("VulDataRepository FindChecked"), false);
	checkf(Row != nullptr, TEXT("Cannot find row %s in table %s"), *TableName.ToString(), *RowName.ToString());

	InitStruct(Table, Table->RowStruct, Row);

	return Row;
}

template <typename RowType>
TVulDataPtr<RowType> UVulDataRepository::FindChecked(const FName& TableName, const FName& RowName)
{
	auto Ptr = FindPtrChecked(TableName, RowName);

	// This assumes TVulDataPtr is the same size as FVulDataPtr.
	static_assert(sizeof(TVulDataPtr<FTableRowBase>) == sizeof(FVulDataPtr), "FVulDataPtr and TVulDataPtr must be the same size");
	return *reinterpret_cast<TVulDataPtr<RowType>*>(&Ptr);
}

inline void UVulDataRepository::InitStruct(const UDataTable* Table, UScriptStruct* Struct, void* Data)
{
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		if (IsPtrType(*It))
		{
			const auto RefProperty = static_cast<FVulDataPtr*>(It->ContainerPtrToValuePtr<void>(Data));
			InitPtrProperty(*It, RefProperty, Struct);
		} else if (const auto ArrayProperty = CastField<FArrayProperty>(*It))
		{
			FScriptArrayHelper Helper(ArrayProperty, It->ContainerPtrToValuePtr<void>(Data));

			for (int i = 0; i < Helper.Num(); ++i)
			{
				auto InnerData = Helper.GetElementPtr(i);

				if (IsPtrType(ArrayProperty->Inner))
				{
					InitPtrProperty(ArrayProperty, reinterpret_cast<FVulDataPtr*>(InnerData), Struct);
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

				if (IsPtrType(MapProperty->KeyProp))
				{
					InitPtrProperty(MapProperty, reinterpret_cast<FVulDataPtr*>(ValueData), Struct);
				} else if (auto ValueProp = CastField<FStructProperty>(MapProperty->ValueProp))
				{
					InitStruct(Table, ValueProp->Struct, ValueData);
				}
			}
		}
	}
}