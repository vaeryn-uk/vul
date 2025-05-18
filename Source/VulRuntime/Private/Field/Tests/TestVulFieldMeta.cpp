#include "TestCase.h"
#include "TestVulFieldStructs.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestVulFieldMeta,
	"VulRuntime.Field.TestVulFieldMeta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestVulFieldMeta::RunTest(const FString& Parameters)
{
	VulTest::Case(this, "Schema generation - basic object", [](VulTest::TC TC)
	{
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, Ctx.Describe<FVulTestFieldType>(Desc), "could not describe FVulTestFieldType");

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
}
)";
		
		(void)TC.JsonObjectsEqual(JsonSchema, Expected);
	});
	
	VulTest::Case(this, "Schema generation - nested objects", [](VulTest::TC TC)
	{
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, Ctx.Describe<FVulTestFieldParent>(Desc), "could not describe FVulTestFieldParent");

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
}
)";
		
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
	
	return true;
}
