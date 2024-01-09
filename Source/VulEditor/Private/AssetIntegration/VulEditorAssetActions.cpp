#include "AssetIntegration/VulEditorAssetActions.h"
#include "DataTable/VulDataRepository.h"
#include "DataTable/VulDataTableSource.h"

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