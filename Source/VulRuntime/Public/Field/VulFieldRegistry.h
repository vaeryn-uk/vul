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
		/* Do not mutate FEntrys directly; see VULFLD_ macros */
		
		FEntry& SetDiscriminatorField(const FString& Field)
		{
			DiscriminatorField = Field;

			return *this;
		}
		
		template <typename Enum> requires HasEnumToString<Enum>
		FEntry& SetDiscriminatorEnumValue(const Enum Value)
		{
			DiscriminatorValue = [Value] { return EnumToString(Value); };
			return *this;
		}
		
		template <typename Base>
		FEntry& SetDerivedFrom()
		{
			Get().Require<Base>();

			BaseType = VulRuntime::Field::TypeId<Base>();
			
			return *this;
		}

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
	TOptional<FEntry> GetBaseType(const FString& TypeId) const;
	
	template <typename T>
	FEntry& Register(const FString& TypeName)
	{
		return Entries.Add(VulRuntime::Field::TypeId<T>(), FEntry{
			.Name = TypeName,
			.TypeId = VulRuntime::Field::TypeId<T>(),
			.DescribeFn = [](FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
			{
				return Ctx.Describe<T>(Description);
			}
		});
	}
	
	template <typename T>
	FEntry& Require()
	{
		checkf(
			Get().Has<T>(),
			TEXT("Type is not registered: %s"),
			*VulRuntime::Field::TypeInfo<T>()
		);

		return Entries[VulRuntime::Field::TypeId<T>()];
	}

private:
	TMap<FString, FEntry> Entries;
};

/**
 * Register CPP TYPE with TYPE_NAME with the Vul Field system.
 */
#define VULFLD_TYPE(TYPE, TYPE_NAME) \
VUL_RUN_ONCE({ \
	(FVulFieldRegistry::Get().Register<TYPE>(TYPE_NAME)); \
})

/**
 * Associates a named field to an already-registered CPP TYPE as it's discriminator: the value
 * which distinguishes which subtype of an abstract base each instance is.
 */
#define VULFLD_DISCRIMINATOR_FIELD(TYPE, DISCRIMINATOR_FIELD) \
VUL_RUN_ONCE({ \
	(FVulFieldRegistry::Get().Require<TYPE>().SetDiscriminatorField(DISCRIMINATOR_FIELD)); \
})

/**
 * Register CPP TYPE with TYPE_NAME which is a derived from already-registered BASE_TYPE with the Vul Field system.
 */
#define VULFLD_DERIVED_TYPE(TYPE, TYPE_NAME, BASE_TYPE) \
VUL_RUN_ONCE({ \
	(FVulFieldRegistry::Get().Register<TYPE>(TYPE_NAME).SetDerivedFrom<BASE_TYPE>()); \
})

/**
 * Binds the already-registered derived CPP TYPE to have a discriminator field value of ENUM_VALUE.
 *
 * Expects ENUM_VALUE to belong to a UENUM with EnumToString defined.
 */
#define VULFLD_DERIVED_DISCRIMINATOR(TYPE, ENUM_VALUE) \
VUL_RUN_ONCE({ \
	(FVulFieldRegistry::Get().Require<TYPE>().SetDiscriminatorEnumValue(ENUM_VALUE)); \
})