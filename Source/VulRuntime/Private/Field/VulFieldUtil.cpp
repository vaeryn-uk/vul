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

FString VulRuntime::Field::PathStr(const TArray<FPathItem>& Path)
{
	if (Path.IsEmpty())
	{
		return ".";
	}
	
	FString Out;

	for (const auto Item : Path)
	{
		if (Item.IsType<FString>())
		{
			Out += "." + Item.Get<FString>();
		} else if (Item.IsType<int>())
		{
			Out += FString::Printf(TEXT("[%d]"), Item.Get<int>());
		}
	}

	return Out;
}

bool VulRuntime::Field::PathMatch(const FPath& Path, const FString& Match)
{
	if (Match.IsEmpty())
	{
		return false;
	}
	
	int StrIndex = 0;
	
	for (const auto Item : Path)
	{
		if (Match.Len() - 1 < StrIndex)
		{
			// More items that the path. Cannot match.
			return false;
		}
		
		if (Item.IsType<FString>() && Match[StrIndex] == '.')
		{
			StrIndex++;
			
			if (Match[StrIndex] == TEXT('*'))
			{
				StrIndex++;
				continue;
			}

			const auto Part = Item.Get<FString>();

			if (Match.Mid(StrIndex, Part.Len()) == Part)
			{
				StrIndex += Part.Len();
				continue;
			}
		} else if (Item.IsType<int>())
		{
			if (Match[StrIndex] == TEXT('['))
			{
				StrIndex++;
				if (Match[StrIndex] == TEXT('*') && Match[StrIndex + 1] == TEXT(']'))
				{
					StrIndex += 2;
					continue;
				}

				FString NumericCharacters;

				while (FChar::IsDigit(Match[StrIndex]))
				{
					NumericCharacters += Match[StrIndex];
					StrIndex++;
				}

				if (Match[StrIndex] == TEXT(']'))
				{
					StrIndex++;
					if (FString::FromInt(Item.Get<int>()) == NumericCharacters)
					{
						continue;
					}
				}
			}
		}

		return false;
	}

	return true;
}

FString VulRuntime::Field::JsonTypeToString(const EJson Type)
{
	switch (Type)
	{
	case EJson::None: return TEXT("none");
	case EJson::Null: return TEXT("null");
	case EJson::String: return TEXT("string");
	case EJson::Number: return TEXT("number");
	case EJson::Boolean: return TEXT("boolean");
	case EJson::Array: return TEXT("array");
	case EJson::Object: return TEXT("object");
	default: return TEXT("Unknown");
	}
}
