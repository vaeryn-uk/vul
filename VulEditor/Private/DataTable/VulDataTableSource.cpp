#include "DataTable/VulDataTableSource.h"
#include "VulEditorUtil.h"
#include "UnrealYAML/Public/Node.h"
#include "UnrealYAML/Public/Parsing.h"
#include "HAL/FileManagerGeneric.h"

void UVulDataTableSource::Import()
{
	if (!EnsureConfigured(true))
	{
		return;
	}

	DataTable->EmptyTable();
}

void UVulDataTableSource::Test()
{
	if (!EnsureConfigured(true))
	{
		return;
	}

	ImportResults = NewObject<UVulDataTableSourceImportResult>(this, UVulDataTableSourceImportResult::StaticClass(), FName(TEXT("Test results")));

	TArray<FString> Paths;

	FFileManagerGeneric::Get().IterateDirectory(*Directory.Path, [this](const TCHAR* Name, bool IsDir)
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
				TMap<FString, TSharedPtr<FJsonValue>> Records;
				if (ParseFile(Name, ParseError, Records))
				{
					TArray<FVulDataTableRow> Rows;

					FString StructError;
					if (BuildStructRows(Records, StructError, Rows))
					{
						Result.NumberOfRows = Rows.Num();

						if (Result.NumberOfRows > 0)
						{
							Result.Ok = true;
						} else
						{
							Result.Error = TEXT("No records found in JSON data");
						}
					} else
					{
						Result.Error = StructError;
					}
				} else
				{
					Result.Error = ParseError;
				}

				ImportResults->Files.Add(Filename, Result);
			}
		}

		return true;
	});

	if (ImportResults->Files.Num() == 0)
	{
		ImportResults->Error = TEXT("No files to import");
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

bool UVulDataTableSource::EnsureConfigured(const bool ShowDialog) const
{
	if (!IsValid(DataTable))
	{
		FVulEditorUtil::Output(INVTEXT("Vul Data Table Source"), INVTEXT("No data table is set"), EAppMsgCategory::Error, ShowDialog);
		return false;
	}

	return true;
}

bool UVulDataTableSource::ParseFile(const FString& Path, FString& Error, TMap<FString, TSharedPtr<FJsonValue>>& Out)
{
	FYamlNode Node;
	UYamlParsing::LoadYamlFromFile(Path, Node);

	Node.IsMap()

	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *Path))
	{
		Error = FString::Printf(TEXT("Could not read file %s"), *Path);
		return false;
	}

	TSharedPtr<FJsonObject> ParsedData;
	{
		const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Content);
		if (!FJsonSerializer::Deserialize(JsonReader, ParsedData))
		{
			Error = FString::Printf(TEXT("Data was not a valid, root-level JSON object."));
			return false;
		}
	}

	Out = ParsedData->Values;

	return true;
}

bool UVulDataTableSource::BuildStructRows(
	const TMap<FString, TSharedPtr<FJsonValue>>& Data,
	FString& Error,
	TArray<FVulDataTableRow>& Out)
{
	for (auto Entry : Data)
	{
		auto RowName = Entry.Key;


	}

	return true;
}
