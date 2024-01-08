#include "VulEditor.h"

#define LOCTEXT_NAMESPACE "FVulEditorModule"

void FVulEditorModule::StartupModule()
{
	DataTableSourceAssetTypeActions = MakeShared<FVulDataTableSourceAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(DataTableSourceAssetTypeActions.ToSharedRef());
}

void FVulEditorModule::ShutdownModule()
{
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return;
	}

	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(DataTableSourceAssetTypeActions.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FVulEditorModule, VulEditor)

DEFINE_LOG_CATEGORY(LogVulEditor)