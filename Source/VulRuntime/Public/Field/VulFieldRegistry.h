#pragma once

#include "CoreMinimal.h"
#include "Field/VulFieldMeta.h"
#include "Field/VulFieldUtil.h"
#include "Field/VulFieldSerializationContext.h"
#include "UObject/Object.h"
#include "VulRuntime.h"

struct FVulFieldSerializationContext;

struct FVulFieldRegistry
{
	struct FEntry {
		FString Name;
		FString TypeId;
		TOptional<FString> DiscriminatorField;
		/**
		 * Fn to return the discriminator value. Deferred execution as this is defined at init-time
		 * and UE may not be ready for enum lookups.
		 */
		TOptional<TFunction<FString ()>> DiscriminatorValue;
		TOptional<FString> BaseType;
		TFunction<bool (FVulFieldSerializationContext&, TSharedPtr<FVulFieldDescription>&)> DescribeFn;
	};

	static FVulFieldRegistry& Get() {
		static FVulFieldRegistry Registry;
		return Registry;
	}

	template <typename T>
	TOptional<FEntry> GetType() const
	{
		return GetType(VulRuntime::Field::TypeId<T>());
	}
	
	TOptional<FEntry> GetType(const FString& TypeId) const
	{
		if (Entries.Contains(TypeId))
		{
			return Entries[TypeId];
		}

		return {};
	}

	template <typename T>
	bool Has() const
	{
		return Has(VulRuntime::Field::TypeId<T>());
	}
	
	bool Has(const FString& TypeId) const
	{
		return GetType(TypeId).IsSet();
	}

	TArray<FEntry> GetSubtypes(const FString& TypeId) const;

	template <typename T>
	void Abstract(const FString& TypeName, const FString& DiscriminatorField)
	{
		Entries.Add(VulRuntime::Field::TypeId<T>(), FEntry{
			.Name = TypeName,
			.TypeId = VulRuntime::Field::TypeId<T>(),
			.DiscriminatorField = DiscriminatorField,
			.DescribeFn = [](FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
			{
				if (Ctx.Describe<T>(Description))
				{
					Description->BindToType<T>();
					return true;
				}

				return false;
			}
		});
	}

	template <typename ThisType, typename BaseType, typename Enum> requires HasEnumToString<Enum>
	void Extends(const FString& TypeName, const Enum Value)
	{
		Entries.Add(VulRuntime::Field::TypeId<ThisType>(), FEntry{
			.Name = TypeName,
			.TypeId = VulRuntime::Field::TypeId<ThisType>(),
			.DiscriminatorValue = [Value] { return EnumToString(Value); },
			.BaseType = VulRuntime::Field::TypeId<BaseType>(),
			.DescribeFn = [](FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
			{
				if (Ctx.Describe<ThisType>(Description))
				{
					Description->BindToType<ThisType>();
					return true;
				}

				return false;
			}
		});
	}

private:
	TMap<FString, FEntry> Entries;
};

#define VUL_FIELD_ABSTRACT(TYPE, TYPE_NAME, DISCRIMINATOR_FIELD) \
VUL_RUN_ONCE({ \
	FVulFieldRegistry::Get().Abstract<TYPE>(TYPE_NAME, DISCRIMINATOR_FIELD); \
})

#define VUL_FIELD_EXTENDS(TYPE, TYPE_NAME, BASE_TYPE, ENUM_VALUE) \
VUL_RUN_ONCE({ \
	(FVulFieldRegistry::Get().Extends<TYPE, BASE_TYPE>(TYPE_NAME, ENUM_VALUE)); \
})