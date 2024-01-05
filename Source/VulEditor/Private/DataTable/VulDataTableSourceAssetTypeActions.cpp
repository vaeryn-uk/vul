#include "DataTable/VulDataTableSourceAssetTypeActions.h"
#include "DataTable/VulDataTableSource.h"

UClass* FVulDataTableSourceAssetTypeActions::GetSupportedClass() const
{
	return UVulDataTableSource::StaticClass();
}

FText FVulDataTableSourceAssetTypeActions::GetName() const
{
	return INVTEXT("Data Table Source");
}

FColor FVulDataTableSourceAssetTypeActions::GetTypeColor() const
{
	return FColor::Cyan;
}

uint32 FVulDataTableSourceAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}
