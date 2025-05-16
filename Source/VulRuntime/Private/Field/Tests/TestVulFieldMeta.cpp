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
	VulTest::Case(this, "Schema generation", [](VulTest::TC TC)
	{
		FVulTestFieldType Field;
		FString JsonSchema;

		FVulFieldSerializationContext Ctx;
		Field.FieldSet().JsonSchema(Ctx, JsonSchema);

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
	
	return true;
}
