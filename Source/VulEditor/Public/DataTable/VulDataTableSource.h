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
	 * If specified, will only process records under this root-level key.
	 *
	 * This can be used to have a single YAML file containing different struct types that each want importing
	 * via their own sources. Allowing a semantic-grouping of YAML elements, rather than strictly a struct ->
	 * file mapping. E.g., instead of having separate files:
	 *   - characters.yaml
	 *   - weapons.yaml
	 *   - pickups.yaml
	 *
	 * You might want to instead define YAML files by some game-specific concept, such as level or character:
	 *   - level01.yaml
	 *   - level02.yaml
	 * Where each file contains the relevant weapons, pickups and characters within that level.
	 *
	 * If specified and a matching file does not contain this root-level key, that file is silently skipped.
	 */
	UPROPERTY(EditAnywhere)
	FString TopLevelKey;

	/**
	 * The name of the row struct our import files should match.
	 *
	 * Read-only: derived from the connected data table.
	 */
	UPROPERTY(VisibleAnywhere)
	FTopLevelAssetPath RowClassName;

	/**
	 * Performs the import, clearing any existing data in the connected data table.
	 */
	UFUNCTION(CallInEditor, Category="Actions")
	bool Import(bool ShowDetails = true);

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
