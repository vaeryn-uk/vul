#pragma once

#include "CoreMinimal.h"
#include "VulDataPtr.h"
#include "UObject/Object.h"
#include "VulDataRepository.generated.h"

/**
 * Data structure used in a reference cache by our data repository.
 */
USTRUCT()
struct FVulDataRepositoryReference
{
	GENERATED_BODY()

	/**
	 * The CPP name of the struct that owns the property that is a reference.
	 */
	UPROPERTY()
	FString PropertyStruct;

	/**
	 * The name of the property is a reference to another table.
	 */
	UPROPERTY()
	FString Property;

	/**
	 * The name of the table that is referenced.
	 */
	UPROPERTY()
	FName ReferencedTable;
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

	virtual void PostLoad() override;

#if WITH_EDITORONLY_DATA
	/**
	 * Builds up a cache of all reference properties across all table managed by this repository.
	 *
	 * This is only available in the editor, as the UPROPERTY meta information that we rely on
	 * is stripped out in game/shipping builds. The idea is that this cache will be built in the
	 * editor, then serialized down with the asset so that it can function in a game build.
	 */
	void RebuildReferenceCache();
	void RebuildReferenceCache(UScriptStruct* Struct);
#endif

	/**
	 * The data tables that make up this repository. Each table is indexed by a user-defined name
	 * which is how a table is identified in ref UPROPERTY metadata specifiers.
	 *
	 * Note that this is a live pointer to keep all data tables in memory always.
	 */
	UPROPERTY(EditAnywhere)
	TMap<FName, UDataTable*> DataTables;

	/**
	 * Caches information about properties that are references to other tables.
	 *
	 * See RebuildReferenceCache.
	 */
	UPROPERTY()
	TArray<FVulDataRepositoryReference> ReferenceCache;

	/**
	 * Has the reference cached been built?
	 */
	UPROPERTY()
	bool ReferencesCached = false;

private:
	friend FVulDataPtr;

	template <typename RowType>
	const RowType* FindRaw(const FName& TableName, const FName& RowName);

	FVulDataPtr FindPtrChecked(const FName& TableName, const FName& RowName);

	void InitStruct(const FName& TableName, const UDataTable* Table, UScriptStruct* Struct, void* Data);

	bool IsPtrType(const FProperty* Property) const;

	/**
	 * Does the provided property reference other tables via a FVulDataPtr, i.e. we expect a
	 * meta=(VulDataTable) definition.
	 *
	 * Checks for map and arrays too.
	 */
	bool IsReferenceProperty(const FProperty* Property) const;

	/**
	 * Returns the struct type associated with this property, if any.
	 *
	 * This checks the property's type, or the contained type for TMap and TArray.
	 */
	UScriptStruct* GetStruct(const FProperty* Property) const;

	void InitPtrProperty(const FName& TableName, const FProperty* Property, FVulDataPtr* Ptr, const UScriptStruct* Struct);
};

template <typename RowType>
const RowType* UVulDataRepository::FindRaw(const FName& TableName, const FName& RowName)
{
	auto Table = DataTables.FindChecked(TableName);
	auto Row = Table->FindRow<RowType>(RowName, TEXT("VulDataRepository FindChecked"), false);
	checkf(Row != nullptr, TEXT("Cannot find row %s in table %s"), *RowName.ToString(), *TableName.ToString());

	InitStruct(TableName, Table, Table->RowStruct, Row);

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
