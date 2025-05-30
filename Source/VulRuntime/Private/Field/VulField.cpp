﻿#include "Field/VulField.h"

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

bool FVulField::Describe(
	FVulFieldSerializationContext& Ctx,
	TSharedPtr<FVulFieldDescription>& Description,
	const TOptional<VulRuntime::Field::FPathItem>& IdentifierCtx
) const {
	return DescribeFn(Ctx, Description, IdentifierCtx);
}
