#pragma once

#include "CoreMinimal.h"
#include "AssetIntegration/VulEditorAssetActions.h"
#include "Modules/ModuleManager.h"

class FVulEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FVulDataTableSourceAssetTypeActions> DataTableSourceAssetTypeActions;
	TSharedPtr<FVulDataRepositoryAssetTypeActions> DataRepositoryAssetTypeActions;
};

DECLARE_LOG_CATEGORY_EXTERN(LogVulEditor, Display, Display)