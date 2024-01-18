#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UnrealYAML/Public/Node.h"
#include "VulDataTableSource.generated.h"

/**
 * Enhanced functionality for importing data in to data tables.
 *
 *   * YAML data file support
 *   * Merge multiple files in to one data table
 *   * Strict validation against data table structures with detailed error reporting.
 *   * Test files before importing.
 */
UCLASS()
class VULEDITOR_API UVulDataTableSource : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * We look for this metadata in UPROPERTY and apply the row name if present.
	 */
	inline static FName RowNameMetaSpecifier = FName("VulRowName");

	/**
	 * The directory in which we look for data files.
	 *
	 * Only files immediately in this directory are considered (no recursive traversal).
	 */
	UPROPERTY(EditAnywhere)
	FDirectoryPath Directory;

	/**
	 * The filename patterns we match on.
	 *
	 * Only the filename (no path info) for each candidate file is checked, e.g. "some_data.json"
	 *
	 * Wildcard syntax is allowed, e.g. "*data.json"
	 */
	UPROPERTY(EditAnywhere)
	TArray<FString> FilePatterns;

	/**
	 * The data table we push data in to.
	 */
	UPROPERTY(EditAnywhere)
	UDataTable* DataTable;

	/**
	 * The name of the row struct our import files should match.
	 *
	 * Read-only: derived fro the connected data table.
	 */
	UPROPERTY(VisibleAnywhere)
	FTopLevelAssetPath RowClassName;

	/**
	 * Performs the import, clearing any existing data in the connected data table.
	 */
	UFUNCTION(CallInEditor, Category="Actions")
	void Import();

	/**
	 * Runs a test, reporting what will happen on import.
	 */
	UFUNCTION(CallInEditor, Category="Actions")
	void Test();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
	void ParseAndBuildRows(TArray<TPair<FName, FTableRowBase*>>& Rows);

	bool EnsureConfigured(const bool ShowDialog) const;

	bool ParseFile(const FString& Path, FString& Error, TMap<FString, YAML::Node>& Out);

	bool BuildStructRows(
		const TMap<FString, YAML::Node>& Data,
		struct FVulDataTableSourceImportFileResult& Result,
		TArray<TPair<FName, FTableRowBase*>>& Rows);

	UPROPERTY()
	class UVulDataTableSourceImportResult* ImportResults;
};

USTRUCT()
struct FVulDataTableSourceImportFileResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	bool Ok = false;

	UPROPERTY(VisibleAnywhere)
	int OkRows = 0;

	UPROPERTY(VisibleAnywhere)
	int FailedRows = 0;

	UPROPERTY(VisibleAnywhere)
	FString PatternMatched;

	UPROPERTY(VisibleAnywhere)
	TArray<FString> Errors;
};

UCLASS()
class UVulDataTableSourceImportResult : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, meta=(ToolTip="Imported files"))
	TMap<FString, FVulDataTableSourceImportFileResult> Files;

	UPROPERTY(VisibleAnywhere)
	int RowCountWouldBeDeleted = 0;

	UPROPERTY(VisibleAnywhere)
	int RowCountActuallyDeleted = 0;

	UPROPERTY(VisibleAnywhere)
	FString Error;

	bool AllFilesOk() const;
};
