#pragma once

#include "CoreMinimal.h"
#include "VulFieldSerializers.h"
#include "UObject/Object.h"

struct FVulField
{
	template <typename T>
	static FVulField Create(T* Ptr)
	{
		FVulField Out;
		Out.Ptr = Ptr;
		Out.Read = [](void* Ptr) { return FVulFieldSerializer<T>::Serialize(*reinterpret_cast<T*>(Ptr)); };
		Out.Write = [](const TSharedPtr<FJsonValue>& Value, void* Ptr)
		{
			return FVulFieldSerializer<T>::Deserialize(Value, *reinterpret_cast<T*>(Ptr));
		};
		return Out;
	}

	bool Set(const TSharedPtr<FJsonValue>& Value) { return Write(Value, Ptr); };
	TSharedPtr<FJsonValue> Get() { return Read(Ptr); };

private:
	void* Ptr = nullptr;
	TFunction<TSharedPtr<FJsonValue> (void*)> Read;
	TFunction<bool (const TSharedPtr<FJsonValue>&, void*)> Write;
};