#pragma once

#include "CoreMinimal.h"
#include "VulFieldRefResolver.h"
#include "VulFieldSerializer.h"
#include "UObject/Object.h"

struct FVulFieldSerializationErrors
{
	bool IsSuccess() const;
	
	template <typename FmtType, typename... Types>
	void Add(const FmtType& Fmt, Types&&... Args)
	{
		Errors.Add(FString::Printf(Fmt, Forward<Types>(Args)...));
	}
	
	template <typename FmtType, typename... Types>
	bool AddIfNot(const bool Condition, const FmtType& Fmt, Types&&... Args)
	{
		if (!Condition)
		{
			Errors.Add(FString::Printf(Fmt, Forward<Types>(Args)...));
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

	TArray<FString> Errors;
};

struct FVulFieldSerializationMemory
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

struct FVulFieldSerializationContext
{
	FVulFieldSerializationErrors Errors;
	FVulFieldSerializationMemory Memory;

	template <typename T>
	bool Serialize(const T& Value, TSharedPtr<FJsonValue>& Out)
	{
		TSharedPtr<FJsonValue> Ref;
		if (!Memory.ResolveRef(Value, Ref, Errors))
		{
			return false;
		}

		if (Ref.IsValid() && Memory.Store.Contains(Ref->AsString()))
		{
			Out = Ref;
			return true;
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
	}
};

struct FVulFieldDeserializationContext
{
	FVulFieldSerializationErrors Errors;
	FVulFieldSerializationMemory Memory;

	template<typename T>
	bool Deserialize(const TSharedPtr<FJsonValue>& Data, T& Out)
	{
		FString PossibleRef;
		if (Data->TryGetString(PossibleRef) && Memory.Store.Contains(PossibleRef))
		{
			Out = *static_cast<T*>(Memory.Store[PossibleRef]);
			return true;
		}
		
		if (!TVulFieldSerializer<T>::Deserialize(Data, Out, *this))
		{
			return false;
		}

		TSharedPtr<FJsonValue> Ref;
		if (!Memory.ResolveRef(Out, Ref, Errors))
		{
			return false;
		}

		if (Ref.IsValid())
		{
			Memory.Store.Add(Ref->AsString(), &Out);
		}

		return true;
	}
};

FString JsonTypeToString(EJson Type)
{
	switch (Type)
	{
	case EJson::None:     return TEXT("None");
	case EJson::Null:     return TEXT("Null");
	case EJson::String:   return TEXT("String");
	case EJson::Number:   return TEXT("Number");
	case EJson::Boolean:  return TEXT("Boolean");
	case EJson::Array:    return TEXT("Array");
	case EJson::Object:   return TEXT("Object");
	default:              return TEXT("Unknown");
	}
}

