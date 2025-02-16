#include "Field/VulFieldSet.h"

void FVulFieldSet::Add(const FVulField& Field, const FString& Identifier, bool ReadOnly)
{
	Fields.Add(Identifier, {.Field = Field, .ReadOnly = ReadOnly, .Identifier = Identifier});
}

bool FVulFieldSet::Serialize(TSharedPtr<FJsonValue>& Out) const
{
	FVulFieldSerializationContext Ctx;
	return Serialize(Out, Ctx);
}

bool FVulFieldSet::Serialize(TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx) const
{
	auto Obj = MakeShared<FJsonObject>();
	
	for (const auto Field : Fields)
	{
		TSharedPtr<FJsonValue> JsonValue;
		if (!Field.Value.Field.Serialize(JsonValue, Ctx))
		{
			return false;
		}
		
		Obj->Values.Add(Field.Key, JsonValue);
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
		if (!Fields.Contains(Entry.Key) || Fields[Entry.Key].ReadOnly)
		{
			continue;
		}

		if (!Fields[Entry.Key].Field.Deserialize(Entry.Value, Ctx))
		{
			return false;
		}
	}

	return true;
}
