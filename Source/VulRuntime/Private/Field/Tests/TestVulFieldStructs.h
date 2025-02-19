#pragma once

#include "CoreMinimal.h"
#include "Field/VulField.h"
#include "Field/VulFieldSet.h"
#include "UObject/Object.h"
#include "Field/VulFieldSet.h"
#include "TestVulFieldStructs.generated.h"

USTRUCT()
struct FVulTestFieldType
{
	GENERATED_BODY()
	
	bool B;
	int I;
	FString S;
	TMap<FString, int> M;
	TArray<bool> A;

	FVulFieldSet FieldSet() const
	{
		FVulFieldSet Out;

		Out.Add(FVulField::Create(&B), "bool");
		Out.Add(FVulField::Create(&I), "int");
		Out.Add(FVulField::Create(&S), "string");
		Out.Add(FVulField::Create(&M), "map");
		Out.Add(FVulField::Create(&A), "array");

		return Out;
	}
};


USTRUCT()
struct FVulTestFieldParent
{
	GENERATED_BODY()
	
	FVulTestFieldType Inner;
	
	FVulFieldSet FieldSet() const
	{
		FVulFieldSet Out;

		Out.Add(FVulField::Create(&Inner), "inner");

		return Out;
	}
};

template<>
struct TVulFieldSerializer<FVulTestFieldType>
{
	static bool Serialize(const FVulTestFieldType& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		return Value.FieldSet().Serialize(Out, Ctx);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, FVulTestFieldType& Out, FVulFieldDeserializationContext& Ctx)
	{
		return Out.FieldSet().Deserialize(Data, Ctx);
	}
};

struct FVulFieldTestTreeBase
{
	virtual ~FVulFieldTestTreeBase() = default;
	
	TArray<TSharedPtr<FVulFieldTestTreeBase>> Children;

	FVulFieldSet VulFieldSet() const
	{
		FVulFieldSet Set;
		AddFields(Set);
		return Set;
	}

	virtual FString Type() const { return "base";}

protected:
	virtual void AddFields(FVulFieldSet& Set) const
	{
		Set.Add<FString>([this] { return Type(); }, "type");
		Set.Add(FVulField::Create(&Children), "children");
	}
};

struct FVulFieldTestTreeNode1 : FVulFieldTestTreeBase
{
	int Int;

	virtual FString Type() const override { return "node1"; }

protected:
	virtual void AddFields(FVulFieldSet& Set) const override
	{
		FVulFieldTestTreeBase::AddFields(Set);
		Set.Add(FVulField::Create(&Int), "int");
	}
};

struct FVulFieldTestTreeNode2 : FVulFieldTestTreeBase
{
	FString String;

	virtual FString Type() const override { return "node2"; }

protected:
	virtual void AddFields(FVulFieldSet& Set) const override
	{
		FVulFieldTestTreeBase::AddFields(Set);
		Set.Add(FVulField::Create(&String), "str");
	}
};

template<>
struct TVulFieldSerializer<TSharedPtr<FVulFieldTestTreeBase>>
{
	static bool Serialize(const TSharedPtr<FVulFieldTestTreeBase>& Value, TSharedPtr<FJsonValue>& Out, FVulFieldSerializationContext& Ctx)
	{
		return Value->VulFieldSet().Serialize(Out, Ctx);
	}

	static bool Deserialize(const TSharedPtr<FJsonValue>& Data, TSharedPtr<FVulFieldTestTreeBase>& Out, FVulFieldDeserializationContext& Ctx)
	{
		TSharedPtr<FJsonValue> TypeStr;
		if (!Ctx.Errors.RequireJsonProperty(Data, "type", TypeStr))
		{
			return false;
		}

		if (TypeStr->AsString() == "base")
		{
			Out = MakeShared<FVulFieldTestTreeBase>();
		} else if (TypeStr->AsString() == "node1")
		{
			Out = MakeShared<FVulFieldTestTreeNode1>();
		} else if (TypeStr->AsString() == "node2")
		{
			Out = MakeShared<FVulFieldTestTreeNode2>();
		} else
		{
			Ctx.Errors.Add(TEXT("invalid type string `%s` for FVulFieldTestTreeBase deserialization"), *TypeStr->AsString());
			return false;
		}

		return Out->VulFieldSet().Deserialize(Data, Ctx);
	}
};

struct FVulFieldTestSingleInstance : IVulFieldSetAware
{
	int Int;
	FString Str;

	FVulFieldSet VulFieldSet() const
	{
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Int), "int");
		Set.Add(FVulField::Create(&Str), "str", true);
		return Set;
	}
};

UCLASS()
class UVulFieldTestUObject2 : public UObject
{
	GENERATED_BODY()
	
public:
	FString Str;
};

UCLASS()
class UVulFieldTestUObject1 : public UObject, public IVulFieldSetAware
{
	GENERATED_BODY()

public:
	FString Str;

	UPROPERTY()
	UVulFieldTestUObject2* Obj;

	virtual FVulFieldSet VulFieldSet() const override
	{
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Str), "str");
		return Set;
	}
};
