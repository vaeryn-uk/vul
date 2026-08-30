#include "DataTable/VulDataPtrCustomization.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DataTable/VulDataRepository.h"
#include "DetailWidgetRow.h"
#include "Engine/DataTable.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FVulDataPtrCustomization::MakeInstance()
{
	return MakeShared<FVulDataPtrCustomization>();
}

void FVulDataPtrCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils
)
{
	RowNameHandle = PropertyHandle->GetChildHandle(TEXT("RowName"));
	TableNameHandle = PropertyHandle->GetChildHandle(TEXT("TableName"));
	RepositoryHandle = PropertyHandle->GetChildHandle(TEXT("Repository"));
	TableNameMeta = FName(PropertyHandle->GetMetaData(TEXT("VulDataTable")));

	TArray<FName> RowNames;
	ResolvedRepository = FindRowNames(TableNameMeta, RowNames);

	RowNameOptions.Reset();
	for (const FName& RowName : RowNames)
	{
		RowNameOptions.Add(MakeShared<FName>(RowName));
	}

	HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()]
		.ValueContent()[SNew(SComboBox<TSharedPtr<FName>>)
							.OptionsSource(&RowNameOptions)
							.OnGenerateWidget_Static(&FVulDataPtrCustomization::OnGenerateRowWidget)
							.OnSelectionChanged(this, &FVulDataPtrCustomization::OnRowSelected)
								[SNew(STextBlock).Text(this, &FVulDataPtrCustomization::GetSelectedRowText)]];
}

UVulDataRepository* FVulDataPtrCustomization::FindRowNames(const FName& TableName, TArray<FName>& OutRowNames)
{
	OutRowNames.Reset();
	if (TableName.IsNone())
	{
		return nullptr;
	}

	const FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> RepositoryAssets;
	AssetRegistryModule.Get().GetAssetsByClass(UVulDataRepository::StaticClass()->GetClassPathName(), RepositoryAssets);

	for (const FAssetData& AssetData : RepositoryAssets)
	{
		UVulDataRepository* Repository = Cast<UVulDataRepository>(AssetData.GetAsset());
		if (Repository == nullptr)
		{
			continue;
		}

		if (UDataTable* const* Table = Repository->DataTables.Find(TableName))
		{
			if (*Table != nullptr)
			{
				OutRowNames = (*Table)->GetRowNames();
				return Repository;
			}
		}
	}

	return nullptr;
}

FText FVulDataPtrCustomization::GetSelectedRowText() const
{
	if (!RowNameHandle.IsValid())
	{
		return FText::GetEmpty();
	}

	FName CurrentRowName;
	RowNameHandle->GetValue(CurrentRowName);
	return CurrentRowName.IsNone() ? FText::FromString(TEXT("(none)")) : FText::FromName(CurrentRowName);
}

void FVulDataPtrCustomization::OnRowSelected(TSharedPtr<FName> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid())
	{
		return;
	}

	if (RowNameHandle.IsValid())
	{
		RowNameHandle->SetValue(*NewSelection);
	}

	if (TableNameHandle.IsValid())
	{
		TableNameHandle->SetValue(TableNameMeta);
	}

	if (RepositoryHandle.IsValid() && ResolvedRepository.IsValid())
	{
		RepositoryHandle->SetValue(static_cast<UObject*>(ResolvedRepository.Get()));
	}
}

TSharedRef<SWidget> FVulDataPtrCustomization::OnGenerateRowWidget(TSharedPtr<FName> InItem)
{
	return SNew(STextBlock).Text(FText::FromName(*InItem));
}
