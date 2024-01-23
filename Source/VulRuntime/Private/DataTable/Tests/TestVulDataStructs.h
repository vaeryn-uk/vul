#pragma once

#include "DataTable/VulDataRepository.h"
#include "TestVulDataStructs.generated.h"

USTRUCT()
struct FTestTableRow1 : public FTableRowBase
{
	GENERATED_BODY()

	FTestTableRow1() = default;
	FTestTableRow1(const int InValue) : Value(InValue) {};

	UPROPERTY()
	int Value = 0;
};

USTRUCT()
struct FTestTableRow2 : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(meta=(VulDataTable="T1"))
	FVulDataPtr ARef;
};

USTRUCT()
struct FCircDepIncluder
{
	GENERATED_BODY()

	FCircDepIncluder() = default;
	FCircDepIncluder(const int InSomeValue, const FVulDataPtr& Ref) : SomeValue(InSomeValue), CircularProperty(Ref) {};

	UPROPERTY()
	int SomeValue = 0;

	UPROPERTY(meta=(VulDataTable="CircTable"))
	FVulDataPtr CircularProperty;
};

USTRUCT()
struct FCircDep : public FTableRowBase
{
	GENERATED_BODY()

	FCircDep() = default;
	FCircDep(const int InValue) : Value(InValue) {};

	UPROPERTY()
	int Value = 0;

	UPROPERTY(meta=(VulDataTable="CircTable"))
	FVulDataPtr CircularProperty;

	UPROPERTY(meta=(VulDataTable="CircTable"))
	TArray<FVulDataPtr> CircularArray;

	UPROPERTY(meta=(VulDataTable="CircTable"))
	TMap<FString, FCircDepIncluder> CircularMap;
};

USTRUCT()
struct FVulTestBaseStruct : public FTableRowBase
{
	GENERATED_BODY()

	FString ParentField;
};

USTRUCT()
struct FVulTestChild1Struct : public FVulTestBaseStruct
{
	GENERATED_BODY()

	FString Child1Field;
};

USTRUCT()
struct FVulTestChild2Struct : public FVulTestBaseStruct
{
	GENERATED_BODY()

	FString Child2Field;
};