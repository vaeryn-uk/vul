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

UENUM()
enum class EVulFieldTestTreeNodeType : uint8
{
	Base,
	Node1,
	Node2,
};

// Substituted-out macro of DECLARE_ENUM_TO_STRING + DEFINE_ENUM_TO_STRING to save needing a .cpp file.
FString EnumToString(const EVulFieldTestTreeNodeType Value)
{
	static const UEnum* TypeEnum = FindObject<UEnum>(nullptr, TEXT("VulRuntime") TEXT(".") TEXT("EVulFieldTestTreeNodeType"));
	return TypeEnum->GetNameStringByIndex(static_cast<int32>(Value));
}

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

	virtual EVulFieldTestTreeNodeType Type() const { return EVulFieldTestTreeNodeType::Base; }

protected:
	virtual void AddFields(FVulFieldSet& Set) const
	{
		Set.Add<EVulFieldTestTreeNodeType>([this] { return Type(); }, "type");
		Set.Add(FVulField::Create(&Children), "children");
	}
};

struct FVulFieldTestTreeNode1 : FVulFieldTestTreeBase
{
	int Int;

	virtual EVulFieldTestTreeNodeType Type() const override { return EVulFieldTestTreeNodeType::Node1; }

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

	virtual EVulFieldTestTreeNodeType Type() const override { return EVulFieldTestTreeNodeType::Node2; }

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

UINTERFACE()
class UVulFieldTestInterface1 : public UInterface
{
	GENERATED_BODY()
};

class IVulFieldTestInterface1
{
	GENERATED_BODY()
};

UINTERFACE()
class UVulFieldTestInterface2 : public UInterface
{
	GENERATED_BODY()
};

class IVulFieldTestInterface2
{
	GENERATED_BODY()
};

UCLASS()
class UVulFieldTestUObject2 : public UObject, public IVulFieldSetAware, public IVulFieldTestInterface1
{
	GENERATED_BODY()
	
public:
	FString Str;

	virtual FVulFieldSet VulFieldSet() const override
	{
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Str), "str", true);
		return Set;
	}
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
		Set.Add(FVulField::Create(&Str), "str", true);
		Set.Add(FVulField::Create(&Obj), "obj");
		return Set;
	}
};
