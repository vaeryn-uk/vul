#pragma once

#include "CoreMinimal.h"
#include "VulFieldMeta.h"
#include "VulFieldRefResolver.h"
#include "VulFieldSerializationOptions.h"
#include "VulFieldSerializer.h"
#include "VulFieldUtil.h"
#include "UObject/Object.h"

struct VULRUNTIME_API FVulFieldSerializationErrors
{
	bool IsSuccess() const;

	/**
	 * Sets the maximum depth de/serialization will traverse before an error and stopping.
	 *
	 * Avoids infinite loops. Default value = 100.
	 */
	void SetMaxStack(int N);
	
	template <typename FmtType, typename... Types>
	void Add(const FmtType& Fmt, Types&&... Args)
	{
		Errors.Add(PathStr() + ": " + FString::Printf(Fmt, Forward<Types>(Args)...));
	}
	
	void Add(const FVulFieldSerializationErrors& Other)
	{
		for (const auto Error : Other.Errors)
		{
			Errors.Add(Error);
		}
	}
	
	template <typename FmtType, typename... Types>
	bool AddIfNot(const bool Condition, const FmtType& Fmt, Types&&... Args)
	{
		if (!Condition)
		{
			Add(Fmt, Forward<Types>(Args)...);
		}

		return Condition;
	}

	/**
	 * Convenience function to check that the provided value is of type, returning false
	 * and adding an error if not.
	 */
	bool RequireJsonType(const TSharedPtr<FJsonValue>& Value, const EJson Type);
	
	bool RequireJsonProperty(
		const TSharedPtr<FJsonValue>& Value,
		const FString& Property,
		TSharedPtr<FJsonValue>& Out,
		const TOptional<EJson> Type = {}
	);

	bool WithIdentifierCtx(const TOptional<VulRuntime::Field::FPathItem>& Identifier, const TFunction<bool ()>& Fn);

	/**
	 * Logs all errors via UE_LOG.
	 */
	void Log();

	TArray<FString> Errors;

	VulRuntime::Field::FPath GetPath() const;

private:
	void Push(const VulRuntime::Field::FPathItem& Identifier);
	void Pop();
	VulRuntime::Field::FPath Stack;
	FString PathStr() const;

	int MaxStackSize = 100;
};

struct VULRUNTIME_API FVulFieldSerializationMemory
{
	TMap<FString, void*> Store;
};

/**
 * Common state for serialization and deserialization operations.
 */
struct VULRUNTIME_API FVulFieldSerializationState
{
	FVulFieldSerializationMemory Memory;
	FVulFieldSerializationErrors Errors;
	
	template <typename T>
	bool ResolveRef(const T& From, TSharedPtr<FJsonValue>& Ref)
	{
		if (const auto HaveRef = TVulFieldRefResolver<T>::Resolve(From, Ref, *this); !HaveRef)
		{
			Ref = nullptr;
			return true;
		}

		FString RefStr;
		if (!Ref->TryGetString(RefStr))
		{
			Errors.Add(TEXT("Resolved a reference that cannot be represented as a JSON string"));
			return false;
		}

		return true;
	}
};

template <typename T>
concept SerializerHasSetup = requires { TVulFieldSerializer<T>::Setup(); };

template <typename T>
concept HasMetaDescribe = requires(FVulFieldSerializationContext& Ctx, const TSharedPtr<FVulFieldDescription>& Description) {
	{ TVulFieldMeta<T>::Describe(Ctx, Description) } -> std::same_as<bool>;
};

struct VULRUNTIME_API FVulFieldSerializationContext
{
	FVulFieldSerializationState State;
	FVulFieldSerializationFlags Flags;

	/**
	 * When serializing floating points, how many decimal places should we include?
	 */
	int DefaultPrecision = 1;

	template <typename T>
	bool Describe(
		const TSharedPtr<FVulFieldDescription>& Description,
		const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx = {}
	) {
#if defined(__clang__) || defined(__GNUC__)
		FString TypeName = ANSI_TO_TCHAR(__PRETTY_FUNCTION__);
#elif defined(_MSC_VER)
		FString TypeName = ANSI_TO_TCHAR(__FUNCSIG__);
#else
		FString TypeName = TEXT("UnknownType");
#endif
		
		return State.Errors.WithIdentifierCtx(IdentifierCtx, [&]
		{
			const auto Result = TVulFieldMeta<T>::Describe(*this, Description);

			if (!Description->IsValid())
			{
				State.Errors.Add(TEXT("TVulFieldMeta::Describe() did not produce a valid description. type info: %s"), *TypeName);
				return false;
			}

			return Result;
		});
	}

	template <typename T>
	bool Serialize(
		const T& Value,
		TSharedPtr<FJsonValue>& Out,
		const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx = {}
	) {
		if constexpr (SerializerHasSetup<T>)
		{
			TVulFieldSerializer<T>::Setup();
		}
		
		return State.Errors.WithIdentifierCtx(IdentifierCtx, [&]
		{
			constexpr bool TypeSupportsRef = TVulFieldRefResolver<T>::SupportsRef;
			const bool SupportsRef = TypeSupportsRef && Flags.IsEnabled(VulFieldSerializationFlag_Referencing, State.Errors.GetPath());
			
			TSharedPtr<FJsonValue> Ref;

			if (SupportsRef)
			{
				if (!State.ResolveRef(Value, Ref))
				{
					return false;
				}

				if (Ref.IsValid() && State.Memory.Store.Contains(Ref->AsString()))
				{
					Out = Ref;
					return true;
				}
			}
			
			if (!TVulFieldSerializer<T>::Serialize(Value, Out, *this))
			{
				return false;
			}

			if (Ref.IsValid())
			{
				State.Memory.Store.Add(Ref->AsString(), &Out);
			}
			
			return true;
		});
	}
};

struct VULRUNTIME_API FVulFieldDeserializationContext
{
	FVulFieldSerializationState State;
	FVulFieldSerializationFlags Flags;

	/**
	 * The outer object we use when deserialization requires creating UObjects.
	 */
	UObject* ObjectOuter = nullptr;

	template<typename T>
	bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out, const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx = {})
	{
		if constexpr (SerializerHasSetup<T>)
		{
			TVulFieldSerializer<T>::Setup();
		}
		
		return State.Errors.WithIdentifierCtx(IdentifierCtx, [&]
		{
			constexpr bool TypeSupportsRef = TVulFieldRefResolver<T>::SupportsRef;
			const bool SupportsRef = TypeSupportsRef && Flags.IsEnabled(VulFieldSerializationFlag_Referencing, State.Errors.GetPath());
			
			if (SupportsRef)
			{
				if constexpr (std::is_copy_assignable_v<T>)
				{
					FString PossibleRef;
					if (Data->TryGetString(PossibleRef) && State.Memory.Store.Contains(PossibleRef))
					{
						Out = *static_cast<T*>(State.Memory.Store[PossibleRef]);
						return true;
					}
				}
			}
			
			if (!TVulFieldSerializer<T>::Deserialize(Data, Out, *this))
			{
				return false;
			}

			if (SupportsRef)
			{
				TSharedPtr<FJsonValue> Ref;
				if (!State.ResolveRef(Out, Ref))
				{
					return false;
				}

				if (Ref.IsValid())
				{
					State.Memory.Store.Add(Ref->AsString(), &Out);
				}
			}

			return true;
		});
	}
};

