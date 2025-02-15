#include "Field/VulFieldSet.h"

void FVulFieldSet::Add(const FVulField& Field, const FString& Identifier, bool ReadOnly)
{
	Fields.Add(Identifier, {.Field = Field, .ReadOnly = ReadOnly, .Identifier = Identifier});
}

bool FVulFieldSet::Serialize(TSharedPtr<FJsonValueObject>& Out) const
{
	auto Obj = MakeShared<FJsonObject>();
	
	for (const auto Field : Fields)
	{
		TSharedPtr<FJsonValue> JsonValue;
		if (!Field.Value.Field.Get(JsonValue))
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

		if (!Fields[Entry.Key].Field.Set(Entry.Value))
		{
			return false;
		}
	}

	return true;
}