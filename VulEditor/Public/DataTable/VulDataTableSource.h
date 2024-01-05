#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DataTable/VulDataTableRow.h"
#include "VulDataTableSource.generated.h"

/**
 *
 */
UCLASS()
class VULEDITOR_API UVulDataTableSource : public UObject
{
	GENERATED_BODY()

public:
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
	bool EnsureConfigured(const bool ShowDialog) const;

	bool ParseFile(const FString& Path, FString& Error, TMap<FString, TSharedPtr<FJsonValue>>& Out);

	bool BuildStructRows(const TMap<FString, TSharedPtr<FJsonValue>>& Data, FString& Error, TArray<FVulDataTableRow>& Out);

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
	int NumberOfRows = 0;

	UPROPERTY(VisibleAnywhere)
	FString PatternMatched;

	UPROPERTY(VisibleAnywhere)
	int NumberOfDuplicates = 0;

	UPROPERTY(VisibleAnywhere)
	FString Error;
};

UCLASS()
class UVulDataTableSourceImportResult : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, meta=(ToolTip="Imported files"))
	TMap<FString, FVulDataTableSourceImportFileResult> Files;

	UPROPERTY(VisibleAnywhere)
	int RowsDeleted = 0;

	UPROPERTY(VisibleAnywhere)
	FString Error;
};
