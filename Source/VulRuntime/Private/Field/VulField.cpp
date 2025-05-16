#include "Field/VulField.h"

bool FVulField::Deserialize(const TSharedPtr<FJsonValue>& Value)
{
	FVulFieldDeserializationContext Ctx;
	return Deserialize(Value, Ctx);
}

bool FVulField::Deserialize(
	const TSharedPtr<FJsonValue>& Value,
	FVulFieldDeserializationContext& Ctx,
	const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
) {
	return Write(Value, Ptr, Ctx, IdentifierCtx);
}

bool FVulField::Serialize(TSharedPtr<FJsonValue>& Out) const
{
	FVulFieldSerializationContext Ctx;
	return Serialize(Out, Ctx);
}

bool FVulField::Serialize(
	TSharedPtr<FJsonValue>& Out,
	FVulFieldSerializationContext& Ctx,
	const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
) const {
	return Read(Ptr, Out, Ctx, IdentifierCtx);
}

bool FVulField::IsReadOnly() const
{
	return bIsReadOnly;
}

void FVulField::Describe(FVulFieldSerializationContext& Ctx, const TSharedPtr<FVulFieldDescription>& Description) const
{
	DescribeFn(Ctx, Description);
}
