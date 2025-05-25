#pragma once

#include "CoreMinimal.h"
#include "VulCopyOnWritePtr.h"
#include "Field/VulFieldSerializationContext.h"
#include "UObject/Object.h"

template <typename T>
concept HasClone = requires(const T& Obj) {
	{ Obj.Clone() } -> std::convertible_to<TSharedPtr<T>>;
};

/**
 * Wraps a TSharedPtr to copy data only when an explicit `Modify` call it made. Useful when
 * often reading but sometimes writing; avoids unnecessary copies.
 *
 * Transparently provides the latest version: either the unmodified original, or a copied
 * version if changes have been made.
 *
 * This wrapper itself acts as a pointer: it can be freely copied and all instances will
 * point to the same TSharedPtr (or copied one) underneath.
 *
 * This can only wrap types that provide a Clone function which is invoked only when the
 * copy occurs. Clone must return a TSharedPtr<T>.
 */
template <HasClone T>
struct TVulCopyOnWritePtr
{
	TVulCopyOnWritePtr() = default;
	TVulCopyOnWritePtr(const TSharedPtr<T>& Ptr)
	{
		Ptrs = MakeShared<FPtrs>();
		Ptrs->Original = Ptr;
	}

	const TSharedPtr<T>& operator*() const { return Resolve(); }
	const T* operator->() const { return Resolve().Get(); }

	TSharedPtr<T> Modify() { return ResolveCopy(); }

	bool IsValid() const { return Ptrs.IsValid() && Resolve().IsValid(); }

private:
	struct FPtrs
	{
		TSharedPtr<T> Original = nullptr;
		TSharedPtr<T> Copied = nullptr;
	};
	
	mutable TSharedPtr<FPtrs> Ptrs = nullptr;

	TSharedPtr<T> ResolveCopy()
	{
		if (!Ptrs.IsValid())
		{
			return nullptr;
		}
		
		if (!Ptrs->Copied.IsValid())
		{
			Ptrs->Copied = Ptrs->Original->Clone();
		}

		return Ptrs->Copied;
	}
	
	TSharedPtr<T> Resolve() const
	{
		if (!Ptrs.IsValid())
		{
			return nullptr;
		}

		return Ptrs->Copied.IsValid() ? Ptrs->Copied : Ptrs->Original;
	}
};

template <typename T>
struct TVulFieldSerializer<TVulCopyOnWritePtr<T>>
{
	static bool Serialize(const TVulCopyOnWritePtr<T>& Value, TSharedPtr<FJsonValue>& Out, struct FVulFieldSerializationContext& Ctx)
	{
		if (!Value.IsValid())
		{
			Out = MakeShared<FJsonValueNull>();
			return true;
		}

		return Ctx.Serialize(*Value, Out);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TVulCopyOnWritePtr<T>& Out, struct FVulFieldDeserializationContext& Ctx)
	{
		// TODO.
		return false;
	}
};

template <typename T>
struct TVulFieldMeta<TVulCopyOnWritePtr<T>>
{
	static bool Describe(FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description)
	{
		if (!Ctx.Describe<T>(Description))
		{
			return false;
		}
		
		Description->Nullable();

		return true;
	}
};