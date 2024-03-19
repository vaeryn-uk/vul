#include "AssetIntegration/VulEditorAssetActions.h"
#include "EditorUtilityLibrary.h"
#include "AssetIntegration/VulEditorCommands.h"
#include "DataTable/VulDataRepository.h"
#include "DataTable/VulDataTableSource.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "VulEditorUtil.h"

void FVulDataRepositoryAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	FAssetTypeActions_Base::GetActions(InObjects, Section);

	Section.Name = FName(TEXT("Vul Data Repository"));

	TSharedPtr<FUICommandList> PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(FVulEditorCommands::Get().ImportAllConnectedSources, FExecuteAction::CreateSP(this, &FVulDataRepositoryAssetTypeActions::ImportAllConnectedSources));

	auto Entry = FToolMenuEntry::InitMenuEntryWithCommandList(FVulEditorCommands::Get().ImportAllConnectedSources, PluginCommands);
	Section.AddEntry(Entry);
}

UClass* FVulDataRepositoryAssetTypeActions::GetSupportedClass() const
{
	return UVulDataRepository::StaticClass();
}

FText FVulDataRepositoryAssetTypeActions::GetName() const
{
	return INVTEXT("Vul Data Repository");
}

FColor FVulDataRepositoryAssetTypeActions::GetTypeColor() const
{
	return FColor::Emerald;
}

uint32 FVulDataRepositoryAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FVulDataRepositoryAssetTypeActions::ImportAllConnectedSources()
{
	auto Repositories = UEditorUtilityLibrary::GetSelectedAssetsOfClass(UVulDataRepository::StaticClass());

	TArray<FString> Failed;
	TArray<FString> Succeeded;
	int Total = 0;

	for (auto RepoAsset : Repositories)
	{
		auto Repo = Cast<UVulDataRepository>(RepoAsset);

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

			for (const auto Entry : Repo->DataTables)
			{
				if (Entry.Value == Loaded->DataTable)
				{
					// This data source is linked to the repository.
					if (!Loaded->Import(false)->AllFilesOk())
					{
						Failed.Add(Loaded->GetPathName());
					} else
					{
						Succeeded.Add(Loaded->GetPathName());
					}
					Total++;
				}
			}
		}
	}

	const auto Title = INVTEXT("Vul data source import");
	const auto Message = FText::FromString(FString::Printf(
		TEXT("%d of %d imports succeeded:\n%s\n%d of %d imports failed:\n%s"),
		Succeeded.Num(),
		Total,
		*FString::Join(Succeeded, TEXT("\n")),
		Failed.Num(),
		Total,
		*FString::Join(Failed, TEXT("\n"))
	));

	if (Total == 0)
	{
		FVulEditorUtil::Output(Title, INVTEXT("No related data sources found"), EAppMsgCategory::Warning, true);
	} else
	{
		FVulEditorUtil::Output(Title, Message, Failed.Num() > 0 ? EAppMsgCategory::Error : EAppMsgCategory::Success, true);
	}
}

UClass* FVulDataTableSourceAssetTypeActions::GetSupportedClass() const
{
	return UVulDataTableSource::StaticClass();
}

FText FVulDataTableSourceAssetTypeActions::GetName() const
{
	return INVTEXT("Vul Data Table Source");
}

FColor FVulDataTableSourceAssetTypeActions::GetTypeColor() const
{
	return FColor::Cyan;
}

uint32 FVulDataTableSourceAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}