#include "Field/VulFieldSet.h"
#include "Field/VulFieldMeta.h"
#include "Field/VulFieldUtil.h"

FVulFieldSet::FEntry& FVulFieldSet::FEntry::EvenIfEmpty(const bool IncludeIfEmpty)
{
	OmitIfEmpty = !IncludeIfEmpty;
	return *this; 
}

TOptional<FString> FVulFieldSet::FEntry::GetTypeId() const
{
	if (TypeId.IsSet())
	{
		return TypeId;
	}

	return Field.GetTypeId();
}

FVulFieldSet::FEntry& FVulFieldSet::Add(const FVulField& Field, const FString& Identifier, const bool IsRef)
{
	FEntry Created;
		
	Created.Field = Field;
		
	if (IsRef)
	{
		RefField = Identifier;
	}

	return Entries.Add(Identifier, Created);
}

void FVulFieldSet::ValidityFn(const TFunction<bool()>& Fn)
{
	IsValidFn = Fn;
}

TSharedPtr<FJsonValue> FVulFieldSet::GetRef(FVulFieldSerializationState& State) const
{
	if (!RefField.IsSet())
	{
		return nullptr;
	}

	// Need a context that shares state for recursive ref resolution & error stacks.
	// This is copied in read-only, but we'll write errors back out to main ctx later.
	FVulFieldSerializationContext Ctx;
	Ctx.State = State;
	
	TSharedPtr<FJsonValue> Ref;

	if (Entries.Contains(RefField.GetValue()))
	{
		if (Entries[RefField.GetValue()].Fn != nullptr)
		{
			Entries[RefField.GetValue()].Fn(Ref, Ctx, VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "__ref_resolution__"));
		} else
		{
			Entries[RefField.GetValue()].Field.Serialize(Ref, Ctx, VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "__ref_resolution__"));
		}
	}

	// Copy errors back.
	State.Errors = Ctx.State.Errors;

	if (!Ref.IsValid())
	{
		State.Errors.Add(TEXT("could not serialize value for ref `%s`"), *RefField.GetValue());
	} else
	{
		return Ref;
	}
	
	return nullptr;
}

bool FVulFieldSet::HasRef() const
{
	return RefField.IsSet();
}

bool FVulFieldSet::IsValid() const
{
	if (IsValidFn == nullptr)
	{
		// Default true if not specified.
		return true;
	}

	return IsValidFn();
}

bool FVulFieldSet::CanBeInvalid() const
{
	return IsValidFn != nullptr;
}

bool FVulFieldSet::Serialize(TSharedPtr<FJsonValue>& Out) const
{
	FVulFieldSerializationContext Ctx;
	return Serialize(Out, Ctx);
}

bool FVulFieldSet::Serialize(TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx) const
{
	if (!IsValid())
	{
		Out = MakeShared<FJsonValueNull>();
		return true;
	}
	
	auto Obj = MakeShared<FJsonObject>();
	
	for (const auto Entry : Entries)
	{
		TSharedPtr<FJsonValue> JsonValue;
		
		if (Entry.Value.Fn != nullptr)
		{
			if (!Entry.Value.Fn(JsonValue, Ctx, VulRuntime::Field::FPathItem(TInPlaceType<FString>(), Entry.Key)))
			{
				return false;
			}
		} else
		{
			if (!Entry.Value.Field.Serialize(JsonValue, Ctx, VulRuntime::Field::FPathItem(TInPlaceType<FString>(), Entry.Key)))
			{
				return false;
			}
		}

		if (Entry.Value.OmitIfEmpty && VulRuntime::Field::IsEmpty(JsonValue))
		{
			continue;
		}
		
		Obj->Values.Add(Entry.Key, JsonValue);
	}

	Out = MakeShared<FJsonValueObject>(Obj);
	return true;
}

bool FVulFieldSet::Deserialize(const TSharedPtr<FJsonValue>& Data)
{
	FVulFieldDeserializationContext Ctx;
	return Deserialize(Data, Ctx);
}

bool FVulFieldSet::Deserialize(const TSharedPtr<FJsonValue>& Data, FVulFieldDeserializationContext& Ctx)
{
	TSharedPtr<FJsonObject>* Obj;
	if (!Data->TryGetObject(Obj))
	{
		return false;
	}

	for (const auto Entry : (*Obj)->Values)
	{
		if (!Entries.Contains(Entry.Key) || Entries[Entry.Key].Fn != nullptr || Entries[Entry.Key].Field.IsReadOnly())
		{
			continue;
		}

		if (!Entries[Entry.Key].Field.Deserialize(Entry.Value, Ctx, VulRuntime::Field::FPathItem(TInPlaceType<FString>(), Entry.Key)))
		{
			return false;
		}
	}

	return true;
}

bool FVulFieldSet::Describe(FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Description) const
{
	for (const auto Entry : Entries)
	{
		TSharedPtr<FVulFieldDescription> Field = MakeShared<FVulFieldDescription>();

		const auto KeyAsPath = VulRuntime::Field::FPathItem(TInPlaceType<FString>(), Entry.Key);
		
		if (Entry.Value.Fn)
		{
			if (!Entry.Value.Describe(Ctx, Field, KeyAsPath))
			{
				return false;
			}
		} else
		{
			if (!Entry.Value.Field.Describe(Ctx, Field, KeyAsPath))
			{
				return false;
			}
		}

		Description->Prop(Entry.Key, Field, !Entry.Value.OmitIfEmpty);
	}

	return true;
}