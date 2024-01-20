#include "VulEditor.h"

#include "AssetIntegration/VulEditorCommands.h"

#define LOCTEXT_NAMESPACE "FVulEditorModule"

void FVulEditorModule::StartupModule()
{
	DataTableSourceAssetTypeActions = MakeShared<FVulDataTableSourceAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(DataTableSourceAssetTypeActions.ToSharedRef());

	DataRepositoryAssetTypeActions = MakeShared<FVulDataRepositoryAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(DataRepositoryAssetTypeActions.ToSharedRef());

	FVulEditorCommands::Register();
}

void FVulEditorModule::ShutdownModule()
{
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return;
	}

	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(DataTableSourceAssetTypeActions.ToSharedRef());
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(DataRepositoryAssetTypeActions.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FVulEditorModule, VulEditor)

DEFINE_LOG_CATEGORY(LogVulEditor)