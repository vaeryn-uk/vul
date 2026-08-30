#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Input/SComboBox.h"

class UVulDataRepository;

// Details-panel picker for FVulDataPtr: a searchable dropdown of row names, in place of typing an FName by
// hand. Which table to offer rows from is read from the property's own meta=(VulDataTable="TableName") --
// the same tag already used by UVulDataRepository::RebuildReferenceCache to validate cross-table references.
//
// Finding rows to list requires finding a UVulDataRepository owning that table in the first place (done by
// scanning the asset registry, since the property alone doesn't say which repository to use). Selecting a row
// writes that same repository into the struct's Repository field alongside RowName/TableName, so the
// resulting FVulDataPtr is immediately resolvable via Get<T>()/EnsurePtr() -- it doesn't depend on being
// nested inside a repository-owned row for UVulDataRepository::InitStruct to wire it up later.
class FVulDataPtrCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> PropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils
	) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils
	) override
	{
	}

private:
	// Finds a UVulDataRepository asset owning a DataTables entry keyed by TableName. Returns that table's row
	// names via OutRowNames, and the owning repository via the return value (nullptr if none found).
	static UVulDataRepository* FindRowNames(const FName& TableName, TArray<FName>& OutRowNames);

	FText GetSelectedRowText() const;
	void OnRowSelected(TSharedPtr<FName> NewSelection, ESelectInfo::Type SelectInfo);
	static TSharedRef<SWidget> OnGenerateRowWidget(TSharedPtr<FName> InItem);

	TSharedPtr<IPropertyHandle> RowNameHandle;
	TSharedPtr<IPropertyHandle> TableNameHandle;
	TSharedPtr<IPropertyHandle> RepositoryHandle;
	FName TableNameMeta;
	TWeakObjectPtr<UVulDataRepository> ResolvedRepository;
	TArray<TSharedPtr<FName>> RowNameOptions;
};
