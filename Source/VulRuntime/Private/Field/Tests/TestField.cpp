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
	VulTest::Case(this, "Field access", [](VulTest::TC TC)
	{
		FVulTestFieldType TestObj = {
			.B = true,
			.I = 13,
			.S = "hello world",
			.M = {{"foo", 13}, {"bar", 14}},
			.A = {true, false, true},
		};
		
		TSharedPtr<FJsonValue> Value;
	
		auto BoolField = FVulField::Create(&TestObj.B);
		TC.Equal(true, BoolField.Serialize(Value), "bool does get");
		TC.Equal(true, Value->AsBool(), "bool does get correctly");
		TC.Equal(true, BoolField.Deserialize(MakeShared<FJsonValueBoolean>(false)), "bool does set");
		TC.Equal(false, TestObj.B, "bool is set correctly");
		TC.Equal(false, BoolField.Deserialize(MakeShared<FJsonValueNumber>(13)), "bool rejects non-bool");
	
		auto IntField = FVulField::Create(&TestObj.I);
		TC.Equal(true, IntField.Serialize(Value), "int does get");
		TC.Equal(13.0, Value->AsNumber(), "int does get correctly");
		TC.Equal(true, IntField.Deserialize(MakeShared<FJsonValueNumber>(26)), "int does set");
		TC.Equal(26, TestObj.I, "int is set correctly");
		TC.Equal(false, IntField.Deserialize(MakeShared<FJsonValueBoolean>(false)), "int rejects non-int");
	
		auto StringField = FVulField::Create(&TestObj.S);
		TC.Equal(true, StringField.Serialize(Value), "str does get");
		TC.Equal(FString("hello world"), Value->AsString(), "str does get correctly");
		TC.Equal(true, StringField.Deserialize(MakeShared<FJsonValueString>("goodbye")), "str does set");
		TC.Equal(FString("goodbye"), TestObj.S, "str is set correctly");
		TC.Equal(false, StringField.Deserialize(MakeShared<FJsonValueBoolean>(false)), "str rejects non-str");
	
		auto MapField = FVulField::Create(&TestObj.M);
		FString MapStr;
		TC.Equal(true, MapField.SerializeToJson(MapStr), "map does get");
		TC.Equal(FString("{\"foo\":13,\"bar\":14}"), MapStr, "map does get correctly");
		TC.Equal(true, MapField.DeserializeFromJson("{\"qux\":15, \"quxx\":16, \"quxxx\": 17}"), "map does set");
		if (TC.Equal(TestObj.M.Num(), 3, "map is set correct: Num()"))
		{
			TC.Equal(TestObj.M.Contains("qux"), true, "map is set correct: qux key");
			TC.Equal(TestObj.M["qux"], 15, "map is set correct: qux value");
			TC.Equal(TestObj.M.Contains("quxx"), true, "map is set correct: quxx key");
			TC.Equal(TestObj.M["quxx"], 16, "map is set correct: quxx value");
			TC.Equal(TestObj.M.Contains("quxxx"), true, "map is set correct: quxxx key");
			TC.Equal(TestObj.M["quxxx"], 17, "map is set correct: quxxx value");
		}
	
		auto ArrayField = FVulField::Create(&TestObj.A);
		FString ArrayStr;
		TC.Equal(true, ArrayField.SerializeToJson(MapStr), "arr does get");
		TC.Equal(FString("[true,false,true]"), MapStr, "arr does get correctly");
		TC.Equal(true, ArrayField.DeserializeFromJson("[false,true]"), "arr does set");
		if (TC.Equal(TestObj.A.Num(), 2, "arr is set correct: Num()"))
		{
			TC.Equal(TestObj.A[0], false, "arr is set correct [0]");
			TC.Equal(TestObj.A[1], true, "arr is set correct [0]");
		}
	});
	
	VulTest::Case(this, "Field set usage", [](VulTest::TC TC)
	{
		FVulTestFieldType TestObj = {
			.B = true,
			.I = 13,
			.S = "hello world",
			.M = {{"foo", 13}, {"bar", 14}},
			.A = {true, false, true},
		};
	
		FString ObjStr;
		TC.Equal(true, TestObj.FieldSet().SerializeToJson(ObjStr), "serialize to json");
		TC.Equal(
			FString("{\"bool\":true,\"int\":13,\"string\":\"hello world\",\"map\":{\"foo\":13,\"bar\":14},\"array\":[true,false,true]}"),
			ObjStr,
			"serialize to json: string correct"
		);
	
		FString NewJson = "{\"bool\":false,\"int\":5,\"string\":\"hi\",\"map\":{\"qux\":10},\"array\":[true, true, true, false]}";
		TC.Equal(true, TestObj.FieldSet().DeserializeFromJson(NewJson), "deserialize from json");
		TC.Equal(false, TestObj.B, "deserialize from json: bool");
		TC.Equal(5, TestObj.I, "deserialize from json: int");
		TC.Equal(FString("hi"), TestObj.S, "deserialize from json: str");
		TC.Equal(TMap<FString, int>{{"qux", 10}}, TestObj.M, "deserialize from json: map");
		TC.Equal(TArray{true, true, true, false}, TestObj.A, "deserialize from json: array");
	});

	VulTest::Case(this, "Nested objects", [](VulTest::TC TC)
	{
		FVulTestFieldParent TestParent = {
			.Inner = {
				.B = true,
				.I = 13,
				.S = "hello world",
				.M = {{"foo", 13}, {"bar", 14}},
				.A = {true, false, true},
			}
		};

		FString ObjStr;
		TC.Equal(true, TestParent.FieldSet().SerializeToJson(ObjStr), "serialize to json");
		TC.Equal(
			FString("{\"inner\":{\"bool\":true,\"int\":13,\"string\":\"hello world\",\"map\":{\"foo\":13,\"bar\":14},\"array\":[true,false,true]}}"),
			ObjStr,
			"serialize to json: string correct"
		);

		FString NewJson = "{\"inner\":{\"bool\":false,\"int\":5,\"string\":\"hi\",\"map\":{\"qux\":10},\"array\":[true, true, true, false]}}";
		TC.Equal(true, TestParent.FieldSet().DeserializeFromJson(NewJson), "deserialize from json");
		TC.Equal(false, TestParent.Inner.B, "deserialize from json: bool");
		TC.Equal(5, TestParent.Inner.I, "deserialize from json: int");
		TC.Equal(FString("hi"), TestParent.Inner.S, "deserialize from json: str");
		TC.Equal(TMap<FString, int>{{"qux", 10}}, TestParent.Inner.M, "deserialize from json: map");
		TC.Equal(TArray{true, true, true, false}, TestParent.Inner.A, "deserialize from json: array");
	});
	
	VulTest::Case(this, "TOptional", [](VulTest::TC TC)
	{
		TOptional<FString> OptStr = {};

		TSharedPtr<FJsonValue> Out;
		TC.Equal(FVulField::Create(&OptStr).Serialize(Out), true, "null does serialize");
		TC.Equal(Out->Type, EJson::Null, "null serialize correctly");

		TC.Equal(FVulField::Create(&OptStr).Deserialize(MakeShared<FJsonValueString>("hello world")), true, "str does deserialize");
		if (TC.Equal(OptStr.IsSet(), true, "str is set"))
		{
			TC.Equal(OptStr.GetValue(), FString("hello world"), "str is set");
		}

		TC.Equal(FVulField::Create(&OptStr).Deserialize(MakeShared<FJsonValueNull>()), true, "null does deserialize");
		TC.Equal(!OptStr.IsSet(), true, "str is not set");
	});
	
	VulTest::Case(this, "Read Only fields", [](VulTest::TC TC)
	{
		struct FTestType
		{
			FString Str1;
			FString Str2;
		};

		FTestType Struct = {.Str1 = "foo", .Str2 = "bar"};

		FVulFieldSet FieldSet;
		FieldSet.Add(FVulField::Create(Struct.Str1), "str1");
		FieldSet.Add(FVulField::Create(&Struct.Str2), "str2");

		FString JsonStr;
		if (TC.Equal(FieldSet.SerializeToJson(JsonStr), true, "serialize"))
		{
			TC.Equal(JsonStr, FString("{\"str1\":\"foo\",\"str2\":\"bar\"}"), "serialize correctly");
		}

		if (TC.Equal(FieldSet.DeserializeFromJson("{\"str1\":\"foo2\",\"str2\":\"bar2\"}"), true, "deserialize"))
		{
			TC.Equal(Struct.Str1, FString("foo"), "deserialize: str1 is unchanged");
			TC.Equal(Struct.Str2, FString("bar2"), "deserialize: str2 is unchanged");
		}

		TC.Equal(FVulField::Create(Struct.Str1).DeserializeFromJson("\"somestr\""), false, "direct deserialize fails");
	});

	
	VulTest::Case(this, "Tree structure", [](VulTest::TC TC)
	{
		TSharedPtr<FVulFieldTestTreeBase> Root = MakeShared<FVulFieldTestTreeBase>();
		TSharedPtr<FVulFieldTestTreeNode1> NodeA = MakeShared<FVulFieldTestTreeNode1>();
		NodeA->Int = 13;
		TSharedPtr<FVulFieldTestTreeNode2> NodeB = MakeShared<FVulFieldTestTreeNode2>();
		NodeB->String = "foo";
		TSharedPtr<FVulFieldTestTreeNode1> NodeC = MakeShared<FVulFieldTestTreeNode1>();
		NodeC->Int = -5;

		NodeB->Children.Add(NodeC);
		Root->Children.Add(NodeB);
		Root->Children.Add(NodeA);

		FString JsonStr;
		if (!TC.Equal(FVulField::Create(&Root).SerializeToJson(JsonStr), true, "serialize"))
		{
			return;
		}
		
		TC.Equal(
			*JsonStr,
			TEXT("{\"children\":[{\"children\":[{\"children\":[],\"int\":-5,\"type\":\"node1\"}],\"str\":\"foo\",\"type\":\"node2\"},{\"children\":[],\"int\":13,\"type\":\"node1\"}],\"type\":\"base\"}"),
			"Json is equal"
		);

		// Deserialize it back in to an empty tree struct.
		TSharedPtr<FVulFieldTestTreeBase> DeserializedRoot = MakeShared<FVulFieldTestTreeBase>();

		FVulFieldDeserializationContext Ctx;
		if (!TC.Equal(FVulField::Create(&DeserializedRoot).DeserializeFromJson(JsonStr, Ctx), true, "deserialize"))
		{
			return;
		}

		// Serialize it again as an easy way to assert the structure is correct.
		FString JsonStr2;
		if (!TC.Equal(FVulField::Create(&Root).SerializeToJson(JsonStr2), true, "serialize again"))
		{
			return;
		}

		TC.Equal(*JsonStr2, *JsonStr, "serialize");
	});
	
	return true;
}
