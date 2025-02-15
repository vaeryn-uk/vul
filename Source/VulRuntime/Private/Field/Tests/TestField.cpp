#include "TestCase.h"
#include "TestVulFieldStructs.h"
#include "Field/VulField.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestField,
	"VulRuntime.Field.TestField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestField::RunTest(const FString& Parameters)
{
	VulTest::Case(this, "Basic field access", [](VulTest::TC TC)
	{
		FVulTestFieldType TestObj = {.B = true, .I = 13, .S = "hello world"};

		auto BoolField = FVulField::Create(&TestObj.B);
		TC.Equal(true, BoolField.Get()->AsBool(), "bool does get");
		TC.Equal(true, BoolField.Set(MakeShared<FJsonValueBoolean>(false)), "bool does set");
		TC.Equal(false, TestObj.B, "bool is set correctly");
		TC.Equal(false, BoolField.Set(MakeShared<FJsonValueNumber>(13)), "bool rejects non-bool");

		auto IntField = FVulField::Create(&TestObj.I);
		TC.Equal(13.0, IntField.Get()->AsNumber(), "int does get");
		TC.Equal(true, IntField.Set(MakeShared<FJsonValueNumber>(26)), "int does set");
		TC.Equal(26, TestObj.I, "int is set correctly");
		TC.Equal(false, IntField.Set(MakeShared<FJsonValueBoolean>(false)), "int rejects non-int");

		auto StringField = FVulField::Create(&TestObj.S);
		TC.Equal(FString("hello world"), StringField.Get()->AsString(), "str does get");
		TC.Equal(true, StringField.Set(MakeShared<FJsonValueString>("goodbye")), "str does set");
		TC.Equal(FString("goodbye"), TestObj.S, "str is set correctly");
		TC.Equal(false, StringField.Set(MakeShared<FJsonValueBoolean>(false)), "str rejects non-str");
	});
	
	return true;
}
