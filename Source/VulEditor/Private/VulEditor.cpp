#include "VulEditor.h"

#include "AssetIntegration/VulEditorCommands.h"

#define LOCTEXT_NAMESPACE "FVulEditorModule"

void FVulEditorModule::StartupModule()
{
	DataTableSourceAssetTypeActions = MakeShared<FVulDataTableSourceAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(DataTableSourceAssetTypeActions.ToSharedRef());

	DataRepositoryAssetTypeActions = MakeShared<FVulDataRepositoryAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(DataRepositoryAssetTypeActions.ToSharedRef());

	ButtonStyleGeneratorAssetTypeActions = MakeShared<FVulButtonStyleGeneratorAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ButtonStyleGeneratorAssetTypeActions.ToSharedRef());

	TextStyleGeneratorAssetTypeActions = MakeShared<FVulTextStyleGeneratorAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(TextStyleGeneratorAssetTypeActions.ToSharedRef());

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
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ButtonStyleGeneratorAssetTypeActions.ToSharedRef());
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(TextStyleGeneratorAssetTypeActions.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FVulEditorModule, VulEditor)

DEFINE_LOG_CATEGORY(LogVulEditor)