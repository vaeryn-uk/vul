#pragma once

#include "CoreMinimal.h"
#include "VulFieldRefResolver.h"
#include "VulFieldSerializationOptions.h"
#include "VulFieldSerializer.h"
#include "UObject/Object.h"

struct VULRUNTIME_API FVulFieldSerializationErrors
{
	bool IsSuccess() const;
	
	template <typename FmtType, typename... Types>
	void Add(const FmtType& Fmt, Types&&... Args)
	{
		Errors.Add(PathStr() + ": " + FString::Printf(Fmt, Forward<Types>(Args)...));
	}
	
	template <typename FmtType, typename... Types>
	bool AddIfNot(const bool Condition, const FmtType& Fmt, Types&&... Args)
	{
		if (!Condition)
		{
			Add(Fmt, Forward<Types>(Args)...);
		}

		return true;
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

	bool WithIdentifierCtx(const TOptional<FString>& Identifier, const TFunction<bool ()>& Fn);

	TArray<FString> Errors;

private:
	void Push(const TOptional<FString>& Identifier);
	void Pop();
	TArray<FString> Stack;
	FString PathStr() const;

	static FString JsonTypeToString(EJson Type);
};

struct VULRUNTIME_API FVulFieldSerializationMemory
{
	template <typename T>
	bool ResolveRef(const T& From, TSharedPtr<FJsonValue>& Ref, FVulFieldSerializationErrors& Errors)
	{
		if (const auto HaveRef = TVulFieldRefResolver<T>::Resolve(From, Ref); !HaveRef)
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
	
	TMap<FString, void*> Store;
};

struct VULRUNTIME_API FVulFieldSerializationContext
{
	FVulFieldSerializationErrors Errors;
	FVulFieldSerializationMemory Memory;
	FVulFieldSerializationFlags Flags;

	template <typename T>
	bool Serialize(const T& Value, TSharedPtr<FJsonValue>& Out, const TOptional<FString>& IdentifierCtx = {})
	{
		return Errors.WithIdentifierCtx(IdentifierCtx, [&]
		{
			constexpr bool TypeSupportsRef = TVulFieldRefResolver<T>::SupportsRef;
			const bool SupportsRef = TypeSupportsRef && Flags.IsEnabled(VulFieldSerializationFlag_Referencing);
			
			TSharedPtr<FJsonValue> Ref;

			if (SupportsRef)
			{
				if (!Memory.ResolveRef(Value, Ref, Errors))
				{
					return false;
				}

				if (Ref.IsValid() && Memory.Store.Contains(Ref->AsString()))
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
				Memory.Store.Add(Ref->AsString(), &Out);
			}
			
			return true;
		});
	}
};

struct VULRUNTIME_API FVulFieldDeserializationContext
{
	FVulFieldSerializationErrors Errors;
	FVulFieldSerializationMemory Memory;
	FVulFieldSerializationFlags Flags;

	/**
	 * The outer object we use when deserialization requires creating UObjects.
	 */
	UObject* ObjectOuter = nullptr;

	template<typename T>
	bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out, const TOptional<FString>& IdentifierCtx = {})
	{
		return Errors.WithIdentifierCtx(IdentifierCtx, [&]
		{
			constexpr bool TypeSupportsRef = TVulFieldRefResolver<T>::SupportsRef;
			const bool SupportsRef = TypeSupportsRef && Flags.IsEnabled(VulFieldSerializationFlag_Referencing);
			
			if (SupportsRef)
			{
				FString PossibleRef;
				if (Data->TryGetString(PossibleRef) && Memory.Store.Contains(PossibleRef))
				{
					Out = *static_cast<T*>(Memory.Store[PossibleRef]);
					return true;
				}
			}
			
			if (!TVulFieldSerializer<T>::Deserialize(Data, Out, *this))
			{
				return false;
			}

			if (SupportsRef)
			{
				TSharedPtr<FJsonValue> Ref;
				if (!Memory.ResolveRef(Out, Ref, Errors))
				{
					return false;
				}

				if (Ref.IsValid())
				{
					Memory.Store.Add(Ref->AsString(), &Out);
				}
			}

			return true;
		});
	}
};

