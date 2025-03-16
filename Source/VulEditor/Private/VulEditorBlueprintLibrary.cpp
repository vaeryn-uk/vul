#include "VulEditorBlueprintLibrary.h"

#include "VulEditor.h"
#include "VulRuntime.h"
#include "DataTable/VulDataTableSource.h"
#include "Subsystems/EditorAssetSubsystem.h"

void UVulEditorBlueprintLibrary::ImportConnectedDataSources(UVulDataRepository* Repo)
{
	TArray<FString> Succeeded;
	TArray<FString> Failed;

	const auto Processed = DoConnectedDataSourceImport(Repo, Succeeded, Failed);

	for (const auto Path : Succeeded)
	{
		UE_LOG(LogVulEditor, Display, TEXT("Imported data source %s"), *Path)
	}

	for (const auto Path : Failed)
	{
		UE_LOG(LogVulEditor, Error, TEXT("Failed to import data source %s"), *Path)
	}

	if (Failed.IsEmpty())
	{
		UE_LOG(LogVulEditor, Display, TEXT("Completed import of %d connected data sources"), Processed)
	} else
	{
		UE_LOG(LogVulEditor, Error, TEXT("Completed import of %d connected data sources with %d failures"), Processed, Failed.Num())
	}
}

int UVulEditorBlueprintLibrary::DoConnectedDataSourceImport(
	UVulDataRepository* Repo,
	TArray<FString>& Succeeded,
	TArray<FString>& Failed
) {
	int Total = 0;
	
	FString Directory;
	FString Name;
	FString Ext;
	FPaths::Split(Repo->GetPathName(), Directory, Name, Ext);

	auto UAS = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	for (auto Asset : UAS->ListAssets(Directory))
	{
		auto Loaded = Cast<UVulDataTableSource>(UAS->LoadAsset(Asset));
		if (!IsValid(Loaded))
		{
			continue;
		}

		// Always rebuild before import incase during dev it's become corrupted.
		Repo->RebuildReferenceCache();

		for (const auto Entry : Repo->DataTables)
		{
			if (Entry.Value == Loaded->DataTable)
			{
				// This data source is linked to the repository.
				if (const auto ImportResult = Loaded->Import(false); !ImportResult->AllFilesOk())
				{
					ImportResult->LogErrors();
					Failed.Add(Loaded->GetPathName());
				} else
				{
					Succeeded.Add(Loaded->GetPathName());
				}
				
				Total++;
			}
		}
	}
	
	return Total;
}
