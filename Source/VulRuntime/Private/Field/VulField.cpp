#include "Field/VulField.h"

bool FVulField::Deserialize(const TSharedPtr<FJsonValue>& Value)
{
	FVulFieldDeserializationContext Ctx;
	return Deserialize(Value, Ctx);
}

bool FVulField::Deserialize(const TSharedPtr<FJsonValue>& Value, FVulFieldDeserializationContext& Ctx)
{
	return Write(Value, Ptr, Ctx);
}

bool FVulField::Serialize(TSharedPtr<FJsonValue>& Out) const
{
	FVulFieldSerializationContext Ctx;
	return Serialize(Out, Ctx);
}

bool FVulField::Serialize(TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx) const
{
	return Read(Ptr, Out, Ctx);
}
