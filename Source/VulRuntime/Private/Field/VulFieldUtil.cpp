#include "Field/VulFieldUtil.h"

bool VulRuntime::Field::IsEmpty(const TSharedPtr<FJsonValue>& Value)
{
	if (!Value.IsValid() || Value->Type == EJson::Null)
	{
		return true;
	}
	
	if (Value->Type == EJson::String)
	{
		return Value->AsString() == "";
	}

	if (Value->Type == EJson::Array)
	{
		for (const auto Item : Value->AsArray())
		{
			if (!IsEmpty(Item))
			{
				return false;
			}
		}

		return true;
	}

	if (Value->Type == EJson::Object)
	{
		for (const auto Entry : Value->AsObject()->Values)
		{
			if (!IsEmpty(Entry.Value))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}
