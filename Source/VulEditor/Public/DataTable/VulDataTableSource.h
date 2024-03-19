#pragma once

#include "CoreMinimal.h"
#include "Parsing.h"
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
	 *
	 * Saves the table if changes were made.
	 */
	UFUNCTION(CallInEditor, Category="Actions", DisplayName="Import")
	void BP_Import();

	/**
	 * Imports data, returning the results in a UOBJECT for further inspection.
	 *
	 * Will render this UObject in an editor dialog to the user if ShowDetails is true.
	 *
	 * If the source is not correctly configured, nullptr will be returned.
	 */
	class UVulDataTableSourceImportResult* Import(bool ShowDetails = false);

	/**
	 * Runs a test, reporting what will happen on import.
	 */
	UFUNCTION(CallInEditor, Category="Actions")
	void Test();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * Registers an additional type handler for parsing YAML nodes into your own project's types.
	 *
	 * Your module's StartupModule function is an easy place to register your own handlers. You
	 * likely want to wrap with an #if WITH_EDITOR directive.
	 */
	static void RegisterAdditionalTypeHandler(const FString& TypeName, const FYamlParseIntoOptions::FTypeHandler& Handler);

private:
	void ParseAndBuildRows(TArray<TPair<FName, FTableRowBase*>>& Rows);

	bool EnsureConfigured(const bool ShowDialog) const;

	bool ParseFile(const FString& Path, FString& Error, TMap<FString, YAML::Node>& Out);

	FString HashTableContents() const;

	bool BuildStructRows(
		const TMap<FString, YAML::Node>& Data,
		struct FVulDataTableSourceImportFileResult& Result,
		TArray<TPair<FName, FTableRowBase*>>& Rows);

	UPROPERTY()
	UVulDataTableSourceImportResult* ImportResults;

	static TMap<FString, FYamlParseIntoOptions::FTypeHandler> AdditionalTypeHandlers;
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
