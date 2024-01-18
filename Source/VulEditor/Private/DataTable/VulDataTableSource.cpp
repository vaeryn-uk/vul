#include "DataTable/VulDataTableSource.h"

#include "EditorAssetLibrary.h"
#include "VulEditorUtil.h"
#include "UnrealYAML/Public/Parsing.h"
#include "HAL/FileManagerGeneric.h"
#include "UObject/PropertyIterator.h"

void UVulDataTableSource::Import()
{
	if (!EnsureConfigured(true))
	{
		return;
	}

	ImportResults = NewObject<UVulDataTableSourceImportResult>(this, UVulDataTableSourceImportResult::StaticClass(), FName(TEXT("Data import")));

	TArray<TPair<FName, FTableRowBase*>> BuiltRows;
	ParseAndBuildRows(BuiltRows);

	if (ImportResults->Error.IsEmpty())
	{
		// Import new data.
		ImportResults->RowCountActuallyDeleted = DataTable->GetRowMap().Num();
		DataTable->EmptyTable();
		for (const auto Entry : BuiltRows)
		{
			DataTable->AddRow(Entry.Key, *Entry.Value);
		}
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(DataTable);
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(DataTable);

	FVulEditorUtil::Output(
		INVTEXT("Data Import"),
		INVTEXT("Import completed"),
		ImportResults->Error.IsEmpty() ? EAppMsgCategory::Success : EAppMsgCategory::Error,
		true,
		ImportResults
	);
}

void UVulDataTableSource::Test()
{
	if (!EnsureConfigured(true))
	{
		return;
	}

	ImportResults = NewObject<UVulDataTableSourceImportResult>(this, UVulDataTableSourceImportResult::StaticClass(), FName(TEXT("Test results")));

	TArray<TPair<FName, FTableRowBase*>> BuiltRows;
	ParseAndBuildRows(BuiltRows);

	// Only testing. Throw away whatever we've built.
	for (const auto BuiltRow : BuiltRows)
	{
		DataTable->RowStruct->DestroyStruct(BuiltRow.Value);
		FMemory::Free(BuiltRow.Value);
	}

	FVulEditorUtil::Output(
		INVTEXT("Data Import [TEST ONLY]"),
		INVTEXT("Test completed"),
		ImportResults->Error.IsEmpty() ? EAppMsgCategory::Success : EAppMsgCategory::Error,
		true,
		ImportResults
	);
}

void UVulDataTableSource::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	if (IsValid(DataTable))
	{
		RowClassName = DataTable->GetRowStructPathName();
	} else
	{
		RowClassName.Reset();
	}
}

void UVulDataTableSource::ParseAndBuildRows(TArray<TPair<FName, FTableRowBase*>>& Rows)
{
	TArray<FString> Paths;

	FFileManagerGeneric::Get().IterateDirectory(*Directory.Path, [this, &Rows](const TCHAR* Name, bool IsDir)
	{
		if (IsDir)
		{
			return true;
		}

		const auto Filename = FPaths::GetCleanFilename(Name);

		for (auto Pattern : FilePatterns)
		{
			if (FString(Filename).MatchesWildcard(Pattern))
			{
				FVulDataTableSourceImportFileResult Result;
				Result.PatternMatched = Pattern;

				FString ParseError;
				TMap<FString, YAML::Node> Records;
				if (ParseFile(Name, ParseError, Records))
				{
					BuildStructRows(Records, Result, Rows);
				} else
				{
					Result.Errors.Add(ParseError);
				}

				if (Result.OkRows + Result.FailedRows == 0)
				{
					Result.Errors.Add(TEXT("No rows to import"));
				}

				Result.Ok = Result.Errors.Num() == 0;

				ImportResults->Files.Add(Filename, Result);
			}
		}

		return true;
	});

	if (ImportResults->Files.Num() == 0)
	{
		ImportResults->Error = TEXT("No files to import");
	} else if (!ImportResults->AllFilesOk())
	{
		ImportResults->Error = TEXT("One or more files encountered an error");
	}

	ImportResults->RowCountWouldBeDeleted = DataTable->GetRowMap().Num();
}

bool UVulDataTableSource::EnsureConfigured(const bool ShowDialog) const
{
	if (!IsValid(DataTable))
	{
		FVulEditorUtil::Output(
			INVTEXT("Vul Data Table Source"),
			INVTEXT("No data table is set"),
			EAppMsgCategory::Error,
			ShowDialog
		);
		return false;
	}

	return true;
}

bool UVulDataTableSource::ParseFile(const FString& Path, FString& Error, TMap<FString, YAML::Node>& Out)
{
	FYamlNode Root;
	if (!UYamlParsing::LoadYamlFromFile(Path, Root))
	{
		Error = FString::Printf(TEXT("Could not parse YAML file %s"), *Path);
		return false;
	}

	if (!Root.IsMap())
	{
		Error = FString::Printf(TEXT("YAML did not contain a root-level map"));
		return false;
	}

	Out = Root.As<TMap<FString, YAML::Node>>();

	return true;
}

bool UVulDataTableSource::BuildStructRows(
	const TMap<FString, YAML::Node>& Data,
	FVulDataTableSourceImportFileResult& Result, TArray<TPair<FName, FTableRowBase*>>& Rows)
{
	bool Ok = true;

	for (auto Entry : Data)
	{
		auto RowName = Entry.Key;

		uint8* RowData = (uint8*)FMemory::Malloc(DataTable->RowStruct->GetStructureSize());
		DataTable->RowStruct->InitializeDefaultValue(RowData);
		FYamlParseIntoCtx ParseResult;
		ParseNodeIntoStruct(
			FYamlNode(Entry.Value),
			DataTable->RowStruct,
			RowData,
			ParseResult,
			FYamlParseIntoOptions::Strict()
		);

		// Check-for and apply row name.
		for (TFieldIterator<FProperty> It(DataTable->RowStruct); It; ++It)
		{
			if (auto NameProp = CastField<FNameProperty>(*It); NameProp && It->HasMetaData(RowNameMetaSpecifier))
			{
				NameProp->SetPropertyValue(It->ContainerPtrToValuePtr<void>(RowData), FName(RowName));
			}
		}

		if (!ParseResult.Success())
		{
			for (auto Error : ParseResult.Errors)
			{
				Result.Errors.Add(FString::Printf(TEXT("%s: %s"), *RowName, *Error));
			}

			Result.FailedRows++;
			Ok = false;
		} else
		{
			Rows.Add(TPair<FName, FTableRowBase*>(RowName, reinterpret_cast<FTableRowBase*>(RowData)));
			Result.OkRows++;
		}
	}

	return Ok;
}

bool UVulDataTableSourceImportResult::AllFilesOk() const
{
	for (const auto File : Files)
	{
		if (!File.Value.Ok)
		{
			return false;
		}
	}

	return true;
}
