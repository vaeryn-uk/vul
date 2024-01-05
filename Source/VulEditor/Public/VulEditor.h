#pragma once

#include "CoreMinimal.h"
#include "DataTable/VulDataTableSourceAssetTypeActions.h"
#include "Modules/ModuleManager.h"

class FVulEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FVulDataTableSourceAssetTypeActions> DataTableSourceAssetTypeActions;
};

DECLARE_LOG_CATEGORY_EXTERN(LogVulEditor, Display, Display)