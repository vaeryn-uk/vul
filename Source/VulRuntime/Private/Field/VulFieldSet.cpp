#include "Field/VulFieldSet.h"

void FVulFieldSet::Add(const FVulField& Field, const FString& Identifier, const bool IsRef)
{
	Fields.Add(Identifier, Field);
	
	if (IsRef)
	{
		RefField = Identifier;
	} 
}

TSharedPtr<FJsonValue> FVulFieldSet::GetRef() const
{
	if (!RefField.IsSet())
	{
		return nullptr;
	}
	
	FVulFieldSerializationContext Ctx;
	TSharedPtr<FJsonValue> Ref;

	if (Fields.Contains(RefField.GetValue()))
	{
		if (Fields[RefField.GetValue()].Serialize(Ref, Ctx))
		{
			return Ref;
		} // TODO: What if we fail to serialize it?
	}

	if (Fns.Contains(RefField.GetValue()))
	{
		if (Fns[RefField.GetValue()](Ref, Ctx))
		{
			return Ref;
		} // TODO: What if we fail to serialize it?
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
	
	for (const auto Entry : Fields)
	{
		TSharedPtr<FJsonValue> JsonValue;
		if (!Entry.Value.Serialize(JsonValue, Ctx))
		{
			return false;
		}
		
		Obj->Values.Add(Entry.Key, JsonValue);
	}
	
	for (const auto Entry : Fns)
	{
		TSharedPtr<FJsonValue> JsonValue;
		if (!Entry.Value(JsonValue, Ctx))
		{
			return false;
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
		if (!Fields.Contains(Entry.Key) || Fields[Entry.Key].IsReadOnly())
		{
			continue;
		}

		if (!Fields[Entry.Key].Deserialize(Entry.Value, Ctx))
		{
			return false;
		}
	}

	return true;
}
