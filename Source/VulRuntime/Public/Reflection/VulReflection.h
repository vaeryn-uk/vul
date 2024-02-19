#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * Utility functions using UE's reflection system.
 *
 * Use with caution.
 */
class VULRUNTIME_API FVulReflection
{
public:
	/**
	 * Sets a UOBJECT's property, circumventing CPP property visibility.
	 *
	 * This is similar to FObjectEditorUtils::SetPropertyValue, but is available at runtime.
	 *
	 * Generally usage should be avoided, but existence is justified for when
	 * a UE private UPROPERTY is freely editable in the editor, but we cannot access it
	 * in CPP (thus requiring manual work).
	 */
	template <typename ValueType>
	bool static SetPropertyValue(UObject* Obj, const FName& PropertyName, ValueType NewValue)
	{
		FProperty* Property = FindFieldChecked<FProperty>(Obj->GetClass(), PropertyName);
		if (!ensureMsgf(Property != nullptr, TEXT("Cannot find DefaultTextStyleOverrideClass")))
		{
			return false;
		}

		ValueType* SourceAddr = Property->ContainerPtrToValuePtr<ValueType>(Obj);
		*SourceAddr = NewValue;

		return true;
	}
};
