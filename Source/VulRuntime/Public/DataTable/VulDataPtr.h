#pragma once

#include "CoreMinimal.h"
#include "Field/VulFieldSet.h"
#include "VulDataPtr.generated.h"


template <typename RowType>
struct TVulDataPtr;
class UVulDataRepository;

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
	FVulDataPtr(nullptr_t) : FVulDataPtr() {}

	/**
	 * A non-null pointer to some row in a table. Do not construct these manually; get one via a UVulDataRepository.
	 */
	FVulDataPtr(const FName& InRowName) : RowName(InRowName) {}

	template <typename T, typename = TEnableIf<TIsDerivedFrom<T, FTableRowBase>::Value>>
	FVulDataPtr(TVulDataPtr<T> Data);

	bool operator==(const FVulDataPtr& Other) const
	{
		return (!IsSet() && !Other.IsSet())
			|| (Other.TableName == TableName && Other.RowName == RowName);
	}

	/**
	 * Returns true if this is a null/non-set ptr.
	 */
	bool IsSet() const;

	FVulFieldSet VulFieldSet() const;

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

	template <typename T, typename = TEnableIf<TIsDerivedFrom<T, FTableRowBase>::Value>>
	TVulDataPtr<T> GetAsDataPtr() const;

	template <typename T, typename = TEnableIf<TIsDerivedFrom<T, FTableRowBase>::Value>>
	const TSharedPtr<T> GetSharedPtr() const
	{
		checkf(
			StructType()->IsChildOf(T::StaticStruct()),
			TEXT("FVulDataPtr: %s is not a %s"),
			*T::StaticStruct()->GetStructCPPName(),
			*StructType()->GetStructCPPName()
		);

		if (SharedPtr == nullptr)
		{
			SharedPtr = MakeShared<T>(*Get<T>());
		}

		return StaticCastSharedPtr<T, void>(SharedPtr);
	}

	FString ToString() const;

	/**
	 * Returns the referenced data if the reference is set, else nullptr.
	 */
	template <typename T, typename = TEnableIf<TIsDerivedFrom<T, FTableRowBase>::Value>>
	const T* GetIfSet() const
	{
		return IsSet() ? Get<T>() : nullptr;
	}

	/**
	 * Ensures ptr data is loaded & ready for use, if valid. See TVulDataPtr::EnsureLoaded.
	 */
	const FVulDataPtr& EnsureLoaded() const
	{
		if (IsValid())
		{
			EnsurePtr();
		}

		return *this;
	}

	const FName& GetRowName() const;
	const FName& GetTableName() const;

	TObjectPtr<UScriptStruct> StructType() const;

	friend class UVulDataRepository;

private:
	FVulDataPtr(
		UVulDataRepository* InRepository,
		const FName& InTableName,
		const FName& InRowName,
		const void* Data = nullptr
	) : Repository(InRepository), TableName(InTableName), RowName(InRowName), Ptr(Data) {}

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
	UVulDataRepository* Repository = nullptr;
	UPROPERTY(VisibleAnywhere)
	FName TableName;
	UPROPERTY(EditAnywhere)
	FName RowName;

	mutable const void* Ptr = nullptr;

	mutable TSharedPtr<void> SharedPtr = nullptr;
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

	TVulDataPtr()
	{
		static_assert(sizeof(TVulDataPtr) == sizeof(FVulDataPtr), "FVulDataPtr and TVulDataPtr must be the same size");
	}
	TVulDataPtr(nullptr_t) {}
	TVulDataPtr(const FVulDataPtr& Other)
	{
		if (Other.IsSet())
		{
			checkf(Other.StructType()->IsChildOf(RowType::StaticStruct()), TEXT("Invalid FVulDataPtr conversion"));
			DataPtr = Other;
		}
	}

	template <typename OtherRowType, typename = TEnableIf<TIsDerivedFrom<OtherRowType, RowType>::Value>>
	TVulDataPtr(const TVulDataPtr<OtherRowType>& Other)
	{
		DataPtr = Other.Data();
	}

	friend bool operator==(const TVulDataPtr& Lhs, const nullptr_t)
	{
		return !Lhs.DataPtr.IsSet();
	}

	friend bool operator!=(const TVulDataPtr& Lhs, const nullptr_t)
	{
		return Lhs.DataPtr.IsSet();
	}

	bool operator==(const TVulDataPtr& Other) const
	{
		return DataPtr == Other.DataPtr;
	}

	/**
	 * Ensures that the underlying pointer is loaded in to memory. Does nothing for an invalid data ptr.
	 *
	 * This is mostly a performance helper. When storing pointers, it can be useful to ensure they're
	 * loaded up front to save callers getting a copy of the pointer having to trigger internal repository
	 * lookups.
	 *
	 * E.g. in your constructor, you may accept a pointer:
	 *
	 * MyClass(const TVulDataPtr<RowData>& InRowData) : MyRowData(InRowData.EnsureLoaded()) {}
	 *
	 * Now later, when we call:
	 * 
	 * TVulDataPtr<RowData> MyClass::GetRowData() const
	 * 
	 * We know we're getting a pre-loaded pointer, saving repeated lookups.
	 */
	const TVulDataPtr& EnsureLoaded() const
	{
		DataPtr.EnsureLoaded();
		return *this;
	}

	FVulFieldSet VulFieldSet() const
	{
		return DataPtr.VulFieldSet();
	}

	/**
	 * Gets the raw pointer to the underlying row data.
	 *
	 * Will trigger a load of the row data if required.
	 */
	const RowType* Get() const
	{
		return DataPtr.Get<RowType>();
	}

	const RowType* operator->() const
	{
		return Get();
	}

	/**
	 * Returns a TSharedPtr from this pointer. This creates a new instance of the row data,
	 * ensuring that the two pointers never interfere with one another. This shared ptr is
	 * cached, thus only 1 extra row data is copied.
	 */
	TSharedPtr<RowType> SharedPtr() const
	{
		return DataPtr.GetSharedPtr<RowType>();
	}

	/**
	 * Returns true if this is capable of returning data (i.e. is not null).
	 */
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
	
	FVulDataPtr& Data()
	{
		return DataPtr;
	}

	/**
	 * Converts an array of untyped to typed pointers.
	 */
	static TArray<TVulDataPtr> Convert(const TArray<FVulDataPtr>& Array)
	{
		TArray<TVulDataPtr> Out;

		for (const auto& Item : Array)
		{
			Out.Add(Item);
		}

		return Out;
	}

private:
	// Must only have this property as we reinterpret_cast between this templated type and the USTRUCT version.
	// I.e. This struct must be the same size as FVulDataPtr.

	FVulDataPtr DataPtr = nullptr;
};

template <typename RowType>
uint32 GetTypeHash(const TVulDataPtr<RowType>& Ptr)
{
	return FCrc::StrCrc32(Ptr.DataPtr.ToString());
}


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

template <typename T, typename>
FVulDataPtr::FVulDataPtr(TVulDataPtr<T> Data)
{
	if (!Data.IsSet())
	{
		return;
	}

	*this = Data.Data();
}

template <typename T, typename>
TVulDataPtr<T> FVulDataPtr::GetAsDataPtr() const
{
	EnsureLoaded(); // Force a load so the returned ptr is ready to use. Saves multiple accesses later.
	return Cast<T>(*this);
}

/**
 * If set, FVulDataPtrs will be serialized as their row name only as a single string.
 */
const static FString VulDataPtr_SerializationFlag_Short = "vul.dataptr.short";

/**
 * If set, TVulDataPtrs will defer to the types internal VulFieldSet() if they have it
 * when serializing. This exports the actual data of the row.
 *
 * Note this is only supported for our typed TVulDataPtrs. FVulDataPtrs will always
 * export as a generic data pointer.
 */
const static FString VulDataPtr_SerializationFlag_Data = "vul.dataptr.data";

template <>
struct TVulFieldSerializer<FVulDataPtr>
{
	static void Setup()
	{
		FVulFieldSerializationFlags::RegisterDefault(VulDataPtr_SerializationFlag_Short, false);
	}
	
	static bool Serialize(const FVulDataPtr& Value, TSharedPtr<FJsonValue>& Out, struct FVulFieldSerializationContext& Ctx)
	{
		if (!Value.IsSet())
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}
		
		if (Ctx.Flags.IsEnabled(VulDataPtr_SerializationFlag_Short, Ctx.State.Errors.GetPath()))
		{
			return Ctx.Serialize(Value.GetRowName(), Out);
		}
		
		return Value.VulFieldSet().Serialize(Out, Ctx);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, FVulDataPtr& Out, struct FVulFieldDeserializationContext& Ctx)
	{
		if (Ctx.Flags.IsEnabled(VulDataPtr_SerializationFlag_Short, Ctx.State.Errors.GetPath()))
		{
			Ctx.State.Errors.Add(TEXT("Cannot deserialize TVulDataPtr with SerializeShort enabled"));
			return false;
		}

		return Out.VulFieldSet().Deserialize(Data, Ctx);
	}
};

template <typename T>
struct TVulFieldSerializer<TVulDataPtr<T>>
{
	static void Setup()
	{
		FVulFieldSerializationFlags::RegisterDefault(VulDataPtr_SerializationFlag_Data, false);
	}
	
	static bool Serialize(const TVulDataPtr<T>& Value, TSharedPtr<FJsonValue>& Out, struct FVulFieldSerializationContext& Ctx)
	{
		if constexpr (HasVulFieldSet<T>)
		{
			if (Ctx.Flags.IsEnabled(VulDataPtr_SerializationFlag_Data, Ctx.State.Errors.GetPath()))
			{
				return Ctx.Serialize<T>(*Value.Get(), Out);
			}
		}
		
		return TVulFieldSerializer<FVulDataPtr>::Serialize(Value.Data(), Out, Ctx);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TVulDataPtr<T>& Out, struct FVulFieldDeserializationContext& Ctx)
	{
		return TVulFieldSerializer<FVulDataPtr>::Deserialize(Data, Out.Data(), Ctx);
	}
};

template <>
struct TVulFieldMeta<FVulDataPtr>
{
	static bool Describe(FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
	{
		if (Ctx.Flags.IsEnabled(VulDataPtr_SerializationFlag_Short, Ctx.State.Errors.GetPath()))
		{
			Description->String();
			return true;
		}

		Ctx.State.Errors.Add(TEXT("Descriptions of FVulDataPtr are not supported with VulDataPtr_SerializationFlag_Short disabled"));

		return false;
	}
};

template <typename T>
struct TVulFieldMeta<TVulDataPtr<T>>
{
	static bool Describe(FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
	{
		return TVulFieldMeta<FVulDataPtr>::Describe(Ctx, Description);
	}
};