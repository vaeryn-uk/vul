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
 *
 * Note that it is expected that this repository persists in memory for as long as its rows
 * are used. Add repositories you use as UPROPERTYs in a game singleton/subsystem, or add
 * this object to the root. Keeping this in memory is critical for the correct behaviour
 * of FVulDataPtr.
 */
UCLASS(BlueprintType)
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

	/**
	 * Convenient row lookup when you expect the row to be found.
	 *
	 * Common scenario when accessing data in CPP from in-editor configuration.
	 */
	template <typename RowType>
	static TVulDataPtr<RowType> Get(
		const TSoftObjectPtr<UVulDataRepository>& Repo,
		const FName& TableName,
		const FName& RowName,
		const FString& ContextString = ""
	);

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

	/**
	 * Loads all rows from the specified table in this repository.
	 *
	 * Returns each row as a lazily-loaded, typed FVulDataPtr.
	 */
	template <typename RowType>
	TArray<TVulDataPtr<RowType>> LoadAllPtrs(const FName& TableName);

private:
	friend FVulDataPtr;

	template <typename RowType>
	const RowType* FindRaw(const FName& TableName, const FName& RowName);
	template <typename RowType>
	const RowType* FindRawChecked(const FName& TableName, const FName& RowName);

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

	void InitPtr(const FName& TableName, FVulDataPtr* Ptr);
};

template <typename RowType>
TArray<TVulDataPtr<RowType>> UVulDataRepository::LoadAllPtrs(const FName& TableName)
{
	const auto Rows = DataTables[TableName]->GetRowNames();

	TArray<TVulDataPtr<RowType>> Ret;

	for (const auto& RowName : Rows)
	{
		auto Ptr = FVulDataPtr(RowName);
		InitPtr(TableName, &Ptr);
		Ret.Add(Ptr);
	}

	return Ret;
}

template <typename RowType>
const RowType* UVulDataRepository::FindRaw(const FName& TableName, const FName& RowName)
{
	auto Table = DataTables.FindChecked(TableName);
	auto Row = Table->FindRow<RowType>(RowName, TEXT("VulDataRepository FindChecked"), false);
	if (Row == nullptr)
	{
		return nullptr;
	}

	InitStruct(TableName, Table, Table->RowStruct, Row);

	return Row;
}

template <typename RowType>
const RowType* UVulDataRepository::FindRawChecked(const FName& TableName, const FName& RowName)
{
	auto Row = FindRaw<RowType>(TableName, RowName);
	checkf(Row != nullptr, TEXT("Cannot find row %s in table %s"), *RowName.ToString(), *TableName.ToString());
	return Row;
}

template <typename RowType>
TVulDataPtr<RowType> UVulDataRepository::FindChecked(const FName& TableName, const FName& RowName)
{
	const RowType* Ptr = FindRawChecked<RowType>(TableName, RowName);

	// This assumes TVulDataPtr is the same size as FVulDataPtr.
	static_assert(sizeof(TVulDataPtr<FTableRowBase>) == sizeof(FVulDataPtr), "FVulDataPtr and TVulDataPtr must be the same size");
	return *reinterpret_cast<TVulDataPtr<RowType>*>(&Ptr);
}

template <typename RowType>
TVulDataPtr<RowType> UVulDataRepository::Get(
	const TSoftObjectPtr<UVulDataRepository>& Repo,
	const FName& TableName,
	const FName& RowName,
	const FString& ContextString
)
{
	const auto LogPrefix = ContextString.IsEmpty() ? "" : FString::Printf(TEXT("%s:"), *ContextString);

	if (!ensureMsgf(!Repo.IsNull(), TEXT("%sCannot Get row: UVulDataRepository is not set"), *LogPrefix))
	{
		return TVulDataPtr<RowType>();
	}

	if (!ensureMsgf(!TableName.IsNone(), TEXT("%sCannot Get row: TableName is not set"), *LogPrefix))
	{
		return TVulDataPtr<RowType>();
	}

	if (!ensureMsgf(!RowName.IsNone(), TEXT("%sCannot Get row: RowName is not set"), *LogPrefix))
	{
		return TVulDataPtr<RowType>();
	}

	const auto LoadedRepo = Repo.LoadSynchronous();
	if (!ensureMsgf(IsValid(LoadedRepo), TEXT("%sCannot Get row: UVulDataRepository cannot be loaded"), *LogPrefix))
	{
		return TVulDataPtr<RowType>();
	}

	if (!ensureMsgf(
		LoadedRepo->DataTables.Contains(TableName),
		TEXT("%sCannot Get row: table %s is not in repo"),
		*LogPrefix,
		*TableName.ToString()
	))
	{
		return TVulDataPtr<RowType>();
	}

	const auto Found = Repo->FindRaw<RowType>(TableName, RowName);
	if (!ensureMsgf(
		Found != nullptr,
		TEXT("%sCannot Get row: row %s is not found in table %s"),
		*LogPrefix,
		*RowName.ToString(),
		*TableName.ToString()
	))
	{
		return TVulDataPtr<RowType>();
	}

	return FVulDataPtr(
		Repo.LoadSynchronous(),
		TableName,
		RowName,
		Found
	);
}
