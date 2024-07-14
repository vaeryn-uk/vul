#include "DataTable/VulDataRepository.h"

TObjectPtr<UScriptStruct> UVulDataRepository::StructType(const FName& TableName) const
{
	return DataTables.FindChecked(TableName)->RowStruct;
}

void UVulDataRepository::PostLoad()
{
	UObject::PostLoad();

#if WITH_EDITORONLY_DATA
	// Trigger a reference build whenever we're loaded in the editor to keep references up to date.
	RebuildReferenceCache();
#endif
}

#if WITH_EDITORONLY_DATA
void UVulDataRepository::RebuildReferenceCache()
{
	ReferenceCache.Reset();

	for (const auto& [Name, Table] : DataTables)
	{
		RebuildReferenceCache(Table->RowStruct);
	}

	ReferencesCached = true;
}

void UVulDataRepository::RebuildReferenceCache(UScriptStruct* Struct)
{
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const auto Property = *It;

		if (IsReferenceProperty(Property))
		{
			checkf(
				Property->HasMetaData(TEXT("VulDataTable")),
				TEXT("%s: meta field VulDataTable must be specified on FVulDataRef properties"),
				*Struct->GetStructCPPName()
			);

			const auto RefTable = FName(Property->GetMetaData(FName(TEXT("VulDataTable"))));
			checkf(DataTables.Contains(RefTable), TEXT("Data repository does not have table %s"), *RefTable.ToString());

			FVulDataRepositoryReference Ref;
			Ref.Property = Property->GetName();
			Ref.ReferencedTable = RefTable;
			Ref.PropertyStruct = Struct->GetStructCPPName();

			ReferenceCache.Add(Ref);

			continue;
		}

		if (const auto StructProperty = GetStruct(Property))
		{
			// This is an embedded struct. Need to check there for more references too.
			RebuildReferenceCache(StructProperty);
		}
	}
}
#endif

FVulDataPtr UVulDataRepository::FindPtrChecked(const FName& TableName, const FName& RowName)
{
	return FVulDataPtr(
		this,
		TableName,
		RowName,
		FindRaw<FTableRowBase>(TableName, RowName)
	);
}

bool UVulDataRepository::IsPtrType(const FProperty* Property) const
{
	return Property->GetCPPType() == FVulDataPtr::StaticStruct()->GetStructCPPName();
}

bool UVulDataRepository::IsReferenceProperty(const FProperty* Property) const
{
	if (IsPtrType(Property))
	{
		return true;
	}

	if (const auto ArrayProperty = CastField<FArrayProperty>(Property); ArrayProperty && IsPtrType(ArrayProperty->Inner))
	{
		return true;
	}

	if (const auto MapProperty = CastField<FMapProperty>(Property);
		MapProperty && (IsPtrType(MapProperty->ValueProp) || IsPtrType(MapProperty->KeyProp)))
	{
		return true;
	}

	return false;
}

UScriptStruct* UVulDataRepository::GetStruct(const FProperty* Property) const
{
	const FProperty* PropertyToCheck;

	if (const auto ArrayProperty = CastField<FArrayProperty>(Property); ArrayProperty)
	{
		PropertyToCheck = ArrayProperty->Inner;
	} else if (const auto MapProperty = CastField<FMapProperty>(Property); MapProperty)
	{
		// TODO: Key support too?
		PropertyToCheck = MapProperty->ValueProp;
	} else
	{
		PropertyToCheck = Property;
	}

	if (const auto StructProperty = CastField<FStructProperty>(PropertyToCheck); StructProperty)
	{
		return StructProperty->Struct;
	}

	return nullptr;
}

void UVulDataRepository::InitPtrProperty(const FName& TableName, const FProperty* Property, FVulDataPtr* Ptr, const UScriptStruct* Struct)
{
	if (!Ptr->IsPendingInitialization())
	{
		// Already initialized or a null ptr.
		return;
	}

	checkf(ReferencesCached, TEXT("Invalid data repository as references have not been cached. Load asset in editor"))

	FName ReferencedTableName;
	for (const auto& Reference : ReferenceCache)
	{
		if (Reference.PropertyStruct == Struct->GetStructCPPName() && Reference.Property == Property->GetName())
		{
			ReferencedTableName = Reference.ReferencedTable;
			break;
		}
	}

	checkf(
		!ReferencedTableName.IsNone(),
		TEXT("Cannot find cached reference for table %s property %s"),
		*TableName.ToString(),
		*Property->GetName()
	);

	InitPtr(ReferencedTableName, Ptr);
}

void UVulDataRepository::InitPtr(const FName& TableName, FVulDataPtr* Ptr)
{
	Ptr->Repository = this;
	Ptr->TableName = TableName;

	checkf(Ptr->IsValid(), TEXT("InitPtr resulted in an invalid FVulDataPtr"))
}

void UVulDataRepository::InitStruct(const FName& TableName, const UDataTable* Table, UScriptStruct* Struct, void* Data)
{
#if WITH_EDITORONLY_DATA
	if (!ReferencesCached)
	{
		RebuildReferenceCache();
	}
#endif

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		if (IsPtrType(*It))
		{
			const auto RefProperty = static_cast<FVulDataPtr*>(It->ContainerPtrToValuePtr<void>(Data));
			InitPtrProperty(TableName, *It, RefProperty, Struct);
		} else if (const auto ArrayProperty = CastField<FArrayProperty>(*It))
		{
			FScriptArrayHelper Helper(ArrayProperty, It->ContainerPtrToValuePtr<void>(Data));

			for (int i = 0; i < Helper.Num(); ++i)
			{
				auto InnerData = Helper.GetElementPtr(i);

				if (IsPtrType(ArrayProperty->Inner))
				{
					InitPtrProperty(TableName, ArrayProperty, reinterpret_cast<FVulDataPtr*>(InnerData), Struct);
				} else if (const auto StructProp = CastField<FStructProperty>(ArrayProperty->Inner))
				{
					InitStruct(TableName, Table, StructProp->Struct, InnerData);
				}
			}
		} else if (const auto MapProperty = CastField<FMapProperty>(*It))
		{
			FScriptMapHelper Helper(MapProperty, It->ContainerPtrToValuePtr<void>(Data));

			for (int i = 0; i < Helper.Num(); ++i)
			{
				// TODO: Support for refs as keys?
				auto ValueData = Helper.GetValuePtr(i);

				if (IsPtrType(MapProperty->KeyProp))
				{
					InitPtrProperty(TableName, MapProperty, reinterpret_cast<FVulDataPtr*>(ValueData), Struct);
				} else if (auto ValueProp = CastField<FStructProperty>(MapProperty->ValueProp))
				{
					InitStruct(TableName, Table, ValueProp->Struct, ValueData);
				}
			}
		} else if (const auto StructProperty = CastField<FStructProperty>(*It))
		{
			// need to seek out more pointers in embedded structs.
			InitStruct(TableName, Table, StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<void>(Data));
		}
	}
}
