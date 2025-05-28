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
	
	TSharedPtr<FJsonObject> Refs;
};

/**
 * Common state for serialization and deserialization operations.
 */
struct VULRUNTIME_API FVulFieldSerializationState
{
	FVulFieldSerializationMemory Memory;
	FVulFieldSerializationErrors Errors;
	TMap<FString, TSharedPtr<FVulFieldDescription>> TypeDescriptions;
	
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
concept HasMetaDescribe = requires(FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description) {
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

	/**
	 * If set, when serialized, references will be separated out in to their own property
	 * and all occurrences will be a reference to that central place.
	 *
	 * These are extracted to a special "__vul_refs" property in serialized output, and
	 * the data itself will be included in a sibling "__vul_data" property.
	 */
	bool ExtractReferences = false;

	/**
	 * Registers the given description pointer with this context, returning true if no errors.
	 *
	 * Usually consumers don't need to worry about this and can just call Describe instead, but
	 * there are rare situations where this registration needs to happen without full Description
	 * recursion.
	 */
	template <typename T>
	bool RegisterDescription(TSharedPtr<FVulFieldDescription>& Description, bool& AlreadyKnown)
	{
		if (State.TypeDescriptions.Contains(VulRuntime::Field::TypeId<T>()))
		{
			Description = State.TypeDescriptions[VulRuntime::Field::TypeId<T>()];
			AlreadyKnown = true;
			return true;
		}

		const auto TypeId = VulRuntime::Field::TypeId<T>(); 
		if (IsKnownType(TypeId))
		{
			Description->BindToType<T>();
				
			if (!State.TypeDescriptions.Contains(TypeId))
			{
				State.TypeDescriptions.Add(TypeId, Description);
			}
				
			if (!GenerateBaseTypeDescription(TypeId, Description))
			{
				return false;
			}
		}

		return true;
	}

	template <typename T>
	bool Describe(
		TSharedPtr<FVulFieldDescription>& Description,
		const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx = {}
	) {
		return State.Errors.WithIdentifierCtx(IdentifierCtx, [&]
		{
			const bool SupportsRef = Flags.SupportsReferencing<T>(State.Errors.GetPath());
			
			bool AlreadyKnown = false;
			if (!RegisterDescription<T>(Description, AlreadyKnown))
			{
				return false;
			}
			
			if (AlreadyKnown)
			{
				if (SupportsRef)
				{
					Description->MaybeRef();
				}
				
				return true;
			}

			const auto Result = TVulFieldMeta<T>::Describe(*this, Description);
			
			if (SupportsRef)
			{
				Description->MaybeRef();
			}

			if (!Description->IsValid())
			{
				State.Errors.Add(
					TEXT("TVulFieldMeta::Describe() did not produce a valid description. type info: %s"),
					*VulRuntime::Field::TypeInfo<T>()
				);
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
			const bool SupportsRef = Flags.SupportsReferencing<T>(State.Errors.GetPath());

			bool IsOuterObject = false;
			if (ExtractReferences && !State.Memory.Refs.IsValid())
			{
				IsOuterObject = true;
				State.Memory.Refs = MakeShared<FJsonObject>();
			}
			
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
				
				if (State.Memory.Refs.IsValid() && !State.Memory.Refs->Values.Contains(Ref->AsString()))
				{
					// If extracting refs, we always output just the ref here, after capturing the full
					// serialized form.
					State.Memory.Refs->Values.Add(Ref->AsString(), Out);
					Out = Ref;
				}
			}

			if (IsOuterObject)
			{
				TSharedPtr<FJsonObject> Return = MakeShared<FJsonObject>();
				Return->Values.Add("refs", MakeShared<FJsonValueObject>(State.Memory.Refs));
				Return->Values.Add("data", Out);
				Out = MakeShared<FJsonValueObject>(Return);
			}
			
			return true;
		});
	}

private:
	bool IsKnownType(const FString& TypeId) const;

	/**
	 * Generate a description for a type if it's a base type with 1 or more subtypes.
	 * 
	 * A OneOf/union of all subtypes.
	 *
	 * Returns true if no error (not necessarily that a base type was generated). 
	 */
	bool GenerateBaseTypeDescription(const FString& TypeId, const TSharedPtr<FVulFieldDescription>& Description);
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
			const bool SupportsRef = Flags.SupportsReferencing<T>(State.Errors.GetPath());
			
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

