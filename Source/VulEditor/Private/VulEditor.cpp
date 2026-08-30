#include "VulEditor.h"

#include "AssetIntegration/VulEditorCommands.h"
#include "DataTable/VulDataPtrCustomization.h"
#include "PropertyEditorModule.h"

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

	BorderStyleGeneratorAssetTypeActions = MakeShared<FVulBorderStyleGeneratorAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(BorderStyleGeneratorAssetTypeActions.ToSharedRef());

	FVulEditorCommands::Register();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		TEXT("VulDataPtr"),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FVulDataPtrCustomization::MakeInstance)
	);
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FVulEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("VulDataPtr"));
	}

	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return;
	}

	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(DataTableSourceAssetTypeActions.ToSharedRef());
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(DataRepositoryAssetTypeActions.ToSharedRef());
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ButtonStyleGeneratorAssetTypeActions.ToSharedRef());
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(TextStyleGeneratorAssetTypeActions.ToSharedRef());
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(BorderStyleGeneratorAssetTypeActions.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVulEditorModule, VulEditor)

DEFINE_LOG_CATEGORY(LogVulEditor)