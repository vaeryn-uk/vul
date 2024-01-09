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
	FVulDataRef ARef;
};

USTRUCT()
struct FCircDepIncluder
{
	GENERATED_BODY()

	FCircDepIncluder() = default;
	FCircDepIncluder(const int InSomeValue, const FVulDataRef& Ref) : SomeValue(InSomeValue), CircularProperty(Ref) {};

	UPROPERTY()
	int SomeValue = 0;

	UPROPERTY(meta=(VulDataTable="CircTable"))
	FVulDataRef CircularProperty;
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
	FVulDataRef CircularProperty;

	UPROPERTY(meta=(VulDataTable="CircTable"))
	TArray<FVulDataRef> CircularArray;

	UPROPERTY(meta=(VulDataTable="CircTable"))
	TMap<FString, FCircDepIncluder> CircularMap;
};