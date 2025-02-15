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
		FVulTestFieldType TestObj = {.B = true, .I = 13, .S = "hello world", .M = {{"foo", 13}, {"bar", 14}}};
		TSharedPtr<FJsonValue> Value;

		auto BoolField = FVulField::Create(&TestObj.B);
		TC.Equal(true, BoolField.Get(Value), "bool does get");
		TC.Equal(true, Value->AsBool(), "bool does get correctly");
		TC.Equal(true, BoolField.Set(MakeShared<FJsonValueBoolean>(false)), "bool does set");
		TC.Equal(false, TestObj.B, "bool is set correctly");
		TC.Equal(false, BoolField.Set(MakeShared<FJsonValueNumber>(13)), "bool rejects non-bool");

		auto IntField = FVulField::Create(&TestObj.I);
		TC.Equal(true, IntField.Get(Value), "int does get");
		TC.Equal(13.0, Value->AsNumber(), "int does get correctly");
		TC.Equal(true, IntField.Set(MakeShared<FJsonValueNumber>(26)), "int does set");
		TC.Equal(26, TestObj.I, "int is set correctly");
		TC.Equal(false, IntField.Set(MakeShared<FJsonValueBoolean>(false)), "int rejects non-int");

		auto StringField = FVulField::Create(&TestObj.S);
		TC.Equal(true, StringField.Get(Value), "str does get");
		TC.Equal(FString("hello world"), Value->AsString(), "str does get correctly");
		TC.Equal(true, StringField.Set(MakeShared<FJsonValueString>("goodbye")), "str does set");
		TC.Equal(FString("goodbye"), TestObj.S, "str is set correctly");
		TC.Equal(false, StringField.Set(MakeShared<FJsonValueBoolean>(false)), "str rejects non-str");

		auto MapField = FVulField::Create(&TestObj.M);
		FString MapStr;
		TC.Equal(true, MapField.ToJsonString(MapStr), "map does get");
		TC.Equal(FString("{\"foo\":13,\"bar\":14}"), MapStr, "map does get correctly");
		TC.Equal(true, MapField.SetFromJsonString("{\"qux\":15, \"quxx\":16, \"quxxx\": 17}"), "map does set");
		if (TC.Equal(TestObj.M.Num(), 3, "map is set correct: Num()"))
		{
			TC.Equal(TestObj.M.Contains("qux"), true, "map is set correct: qux key");
			TC.Equal(TestObj.M["qux"], 15, "map is set correct: qux value");
			TC.Equal(TestObj.M.Contains("quxx"), true, "map is set correct: quxx key");
			TC.Equal(TestObj.M["quxx"], 16, "map is set correct: quxx value");
			TC.Equal(TestObj.M.Contains("quxxx"), true, "map is set correct: quxxx key");
			TC.Equal(TestObj.M["quxxx"], 17, "map is set correct: quxxx value");
		}
	});
	
	return true;
}
