#pragma once

#include "CoreMinimal.h"
#include "VulDataPtr.generated.h"

/**
 * A lightweight pointer to data retrieved from a data repository.
 *
 * Features:
 *   - Use in USTRUCT UPROPERTYs to refer to rows in other tables in the repository.
 *     - Fetch these references with a convenient API.
 *   - Support for up & down casting between row types with static & runtime type safety. See Cast functions below.
 *   - Can be cheaply copied without needing to copy all struct data.
 *   - Support for not-set/null pointer; check with IsSet().
 *   - Efficient network serialization (the struct data does not need to be packaged over the wire). TODO
 */
USTRUCT()
struct VULRUNTIME_API FVulDataPtr
{
	GENERATED_BODY()

	/**
	 * Creates a null/not-set pointer.
	 */
	FVulDataPtr() = default;

	/**
	 * A non-null pointer to some row in a table. Do not construct these manually; get one via a UVulDataRepository.
	 */
	FVulDataPtr(const FName& InRowName) : RowName(InRowName) {}

	/**
	 * Returns true if this is a null/non-set ptr.
	 */
	bool IsSet() const;

	/**
	 * Gets the underlying data as a read-only raw pointer.
	 *
	 * This asserts this is a set & valid pointer.
	 */
	template <typename T, typename = TEnableIf<TIsDerivedFrom<T, FTableRowBase>::Value>>
	const T* Get() const
	{
		checkf(
			StructType()->IsChildOf(T::StaticStruct()),
			TEXT("FVulDataPtr: %s is not a %s"),
			*T::StaticStruct()->GetStructCPPName(),
			*StructType()->GetStructCPPName()
		);

		return static_cast<const T*>(EnsurePtr());
	}

	const FName& GetRowName() const;
	const FName& GetTableName() const;

	TObjectPtr<UScriptStruct> StructType() const;

	friend class UVulDataRepository;

private:
	FVulDataPtr(
		const TSoftObjectPtr<UVulDataRepository>& InRepository,
		const FName& InTableName,
		const FName& InRowName,
		const void* Data = nullptr) : Repository(InRepository),
								TableName(InTableName), RowName(InRowName), Ptr(Data)
	{
	}

	/**
	 * Should this pointer be initialized by a data repository?
	 */
	bool IsPendingInitialization() const;
	/**
	 * True if this has everything needed to load data.
	 *
	 * False for null ptrs.
	 */
	bool IsValid() const;

	const void* EnsurePtr() const;

	UPROPERTY()
	TSoftObjectPtr<UVulDataRepository> Repository;
	UPROPERTY()
	FName TableName;
	UPROPERTY(EditAnywhere)
	FName RowName;

	mutable const void* Ptr = nullptr;
};

/**
 * A templated data ptr when you know what row type you'll be dealing with.
 *
 * Cannot be used in UPROPERTYs, but useful in CPP code.
 */
template <typename RowType>
struct TVulDataPtr
{
	static_assert(TIsDerivedFrom<RowType, FTableRowBase>::Value, "TVulDataPtr RowType must be a FTableRowBase");

	TVulDataPtr() = default;
	TVulDataPtr(const FVulDataPtr& Other)
	{
		checkf(Other.StructType()->IsChildOf(RowType::StaticStruct()), TEXT("Invalid FVulDataPtr conversion"));
		DataPtr = Other;
	}
	template <typename OtherRowType, typename = TEnableIf<TIsDerivedFrom<OtherRowType, RowType>::Value>>
	TVulDataPtr(const TVulDataPtr<OtherRowType>& Other)
	{
		DataPtr = Other.Data();
	}

	/**
	 * Gets the raw pointer to the underlying row data.
	 */
	const RowType* Get() const
	{
		return DataPtr.Get<RowType>();
	}

	const RowType* operator->() const
	{
		return Get();
	}

	bool IsSet() const
	{
		return DataPtr.IsSet();
	}

	/**
	 * Returns the underlying, untyped FVulDataPtr.
	 */
	FVulDataPtr Data() const
	{
		return DataPtr;
	}

	/**
	 * Converts an array of pointers.
	 */
	static TArray<TVulDataPtr> Convert(const TArray<FVulDataPtr>& Array)
	{
		TArray<TVulDataPtr> Out;

		for (const auto Item : Array)
		{
			Out.Add(Item);
		}

		return Out;
	}

private:
	// Must only have this property as we reinterpret_cast between this templated type and the USTRUCT version.
	// I.e. This struct must be the same size as FVulDataPtr.

	FVulDataPtr DataPtr;
};

/**
 * Casts an untyped ptr to a typed one, returning an unset pointer if the conversion
 * cannot be made.
 */
template <typename To, typename = TEnableIf<TIsDerivedFrom<To, FTableRowBase>::Value>>
TVulDataPtr<To> Cast(const FVulDataPtr& Ptr)
{
	if (!Ptr.StructType()->IsChildOf(To::StaticStruct()))
	{
		return TVulDataPtr<To>();
	}

	return Ptr;
}

/**
 * Casts from a typed pointer to another type, returning an unset pointer if the conversion
 * cannot be made.
 */
template <typename To, typename From, typename = TEnableIf<TIsDerivedFrom<To, From>::Value>>
TVulDataPtr<To> Cast(const TVulDataPtr<From>& Ptr)
{
	return Cast<To>(Ptr.Data());
}