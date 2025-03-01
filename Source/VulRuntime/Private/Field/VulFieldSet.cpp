﻿#include "Field/VulFieldSet.h"

#include "Field/VulFieldUtil.h"

FVulFieldSet::FEntry& FVulFieldSet::FEntry::EvenIfEmpty(const bool IncludeIfEmpty)
{
	OmitIfEmpty = !IncludeIfEmpty;
	return *this; 
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

TSharedPtr<FJsonValue> FVulFieldSet::GetRef(FVulFieldSerializationErrors& Errors) const
{
	if (!RefField.IsSet())
	{
		return nullptr;
	}
	
	FVulFieldSerializationContext Ctx;
	TSharedPtr<FJsonValue> Ref;

	if (Entries.Contains(RefField.GetValue()))
	{
		if (Entries[RefField.GetValue()].Fn != nullptr)
		{
			Entries[RefField.GetValue()].Fn(Ref, Ctx, FString("__ref_resolution__"));
		} else
		{
			Entries[RefField.GetValue()].Field.Serialize(Ref, Ctx, FString("__ref_resolution__"));
		}
	}

	// Copy any errors so they can be picked up by outer serialization contexts.
	Errors.Add(Ctx.Errors);

	if (!Ref.IsValid())
	{
		Errors.Add(TEXT("could not serialize value for ref `%s`"), *RefField.GetValue());
	} else
	{
		return Ref;
	}
	
	return nullptr;
}

bool FVulFieldSet::Serialize(TSharedPtr<FJsonValue>& Out) const
{
	FVulFieldSerializationContext Ctx;
	return Serialize(Out, Ctx);
}

bool FVulFieldSet::Serialize(TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx) const
{
	auto Obj = MakeShared<FJsonObject>();
	
	for (const auto Entry : Entries)
	{
		TSharedPtr<FJsonValue> JsonValue;
		if (Entry.Value.Fn != nullptr)
		{
			if (!Entry.Value.Fn(JsonValue, Ctx, Entry.Key))
			{
				return false;
			}
		} else
		{
			if (!Entry.Value.Field.Serialize(JsonValue, Ctx, Entry.Key))
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

		if (!Entries[Entry.Key].Field.Deserialize(Entry.Value, Ctx, Entry.Key))
		{
			return false;
		}
	}

	return true;
}
