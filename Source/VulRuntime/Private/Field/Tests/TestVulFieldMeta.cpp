#include "TestCase.h"
#include "TestVulFieldStructs.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestVulFieldMeta,
	"VulRuntime.Field.TestVulFieldMeta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

template <typename T>
bool TestDescribe(VulTest::TC TC, FVulFieldSerializationContext& Ctx, const TSharedPtr<FVulFieldDescription>& Desc)
{
	const auto Ok = Ctx.Describe<T>(Desc);

	if (!Ok)
	{
		TC.Error("Describe() failed");
		Ctx.State.Errors.Log();
	}

	return Ok;
}

bool TestDescribe(VulTest::TC TC, const FVulFieldSet& Set, FVulFieldSerializationContext& Ctx, const TSharedPtr<FVulFieldDescription>& Desc)
{
	const auto Ok = Set.Describe(Ctx, Desc);

	if (!Ok)
	{
		TC.Error("Describe() FVulFieldSet failed");
		Ctx.State.Errors.Log();
	}

	return Ok;
}

bool TestVulFieldMeta::RunTest(const FString& Parameters)
{
	VulTest::Case(this, "Schema generation - basic object", [](VulTest::TC TC)
	{
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, TestDescribe<FVulTestFieldType>(TC, Ctx, Desc), "");

		const auto JsonSchema = VulRuntime::Field::JsonToString(Desc->JsonSchema());

		FString Expected = R"(
{
	"type": "object",
	"properties": {
		"bool": {"type": "boolean"},
		"int": {"type": "number"},
		"string": {"type": "string"},
		"map": {
			"type": "object",
			"additionalProperties": {"type": "number"}
		},
		"array": {
			"type": "array",
			"items": {"type": "boolean"}
		}
	}
})";
		
		(void)TC.JsonObjectsEqual(JsonSchema, Expected);
	});
	
	VulTest::Case(this, "Schema generation - nested objects", [](VulTest::TC TC)
	{
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, TestDescribe<FVulTestFieldParent>(TC, Ctx, Desc), "");

		const auto JsonSchema = VulRuntime::Field::JsonToString(Desc->JsonSchema());

		FString Expected = R"(
{
	"type": "object",
	"properties": {
		"inner": {
			"type": "object",
			"properties": {
				"bool": {"type": "boolean"},
				"int": {"type": "number"},
				"string": {"type": "string"},
				"map": {
					"type": "object",
					"additionalProperties": {"type": "number"}
				},
				"array": {
					"type": "array",
					"items": {"type": "boolean"}
				}
			}
		}
	}
})";
		
		(void)TC.JsonObjectsEqual(JsonSchema, Expected);
	});
	
	VulTest::Case(this, "Schema generation - type cannot be described", [](VulTest::TC TC)
	{
		class FMyCustomType : FString { };
		
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(false, Ctx.Describe<FMyCustomType>(Desc), "describe failed");

		CtxContainsError(TC, Ctx.State.Errors, "did not produce a valid description");
		CtxContainsError(TC, Ctx.State.Errors, "FMyCustomType");
	});
	
	VulTest::Case(this, "Schema generation - other common types", [](VulTest::TC TC)
	{
		TOptional<FString> Optional;
		TSharedPtr<FString> SharedPtr;
		TUniquePtr<FString> UniquePtr;
		TPair<FString, FString> Pair;
		TPair<FString, int> DifferingPair;
		FString* Ptr;
		FGuid Guid;
		FName Name;
		float Float;
		
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Optional), "optional");
		Set.Add(FVulField::Create(&SharedPtr), "sharedPtr");
		Set.Add(FVulField::Create(&UniquePtr), "uniquePtr");
		Set.Add(FVulField::Create(&Pair), "pair");
		Set.Add(FVulField::Create(&DifferingPair), "differingPair");
		Set.Add(FVulField::Create(&Ptr), "ptr");
		Set.Add(FVulField::Create(&Guid), "guid");
		Set.Add(FVulField::Create(&Name), "name");
		Set.Add(FVulField::Create(&Float), "float");
		
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, TestDescribe(TC, Set, Ctx, Desc), "");

		const auto JsonSchema = VulRuntime::Field::JsonToString(Desc->JsonSchema());

		FString Expected = R"(
{
  "type": "object",
  "properties": {
    "optional": {"type": ["string", "null"]},
    "sharedPtr": {"type": ["string", "null"]},
    "uniquePtr": {"type": ["string", "null"]},
    "pair": {
      "type": "array",
      "items": {"type": "string"}
    },
    "differingPair": {
      "type": "array",
      "items": {
        "oneOf": [
          {"type": "string"},
          {"type": "number"}
        ]
      }
    },
    "ptr": {"type": ["string", "null"]},
    "guid": {"type": "string"},
    "name": {"type": "string"},
    "float": {"type": "number"}
  }
})";
		
		(void)TC.JsonObjectsEqual(JsonSchema, Expected);
	});
	
	VulTest::Case(this, "Schema generation - enum", [](VulTest::TC TC)
	{
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, TestDescribe<EVulFieldTestTreeNodeType>(TC, Ctx, Desc), "");

		const auto JsonSchema = VulRuntime::Field::JsonToString(Desc->JsonSchema());

		FString Expected = R"(
{
  "type": "string",
  "enum": [
    "Base",
    "Node1",
    "Node2"
  ]
})";
		
		(void)TC.JsonObjectsEqual(JsonSchema, Expected);
	});
	
	return true;
}
