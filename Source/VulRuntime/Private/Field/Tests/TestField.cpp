#include "TestCase.h"
#include "TestVulFieldStructs.h"
#include "Field/VulField.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestField,
	"VulRuntime.Field.TestField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool CtxContainsError(VulTest::TC TC, const FVulFieldSerializationErrors& Errors, const FString& Term)
{
	for (const auto Err : Errors.Errors)
	{
		if (Err.Contains(Term))
		{
			return true;
		}
	}

	TC.Error(FString::Printf(
		TEXT("Could not find error term \"%s\" in errors:\n%s"),
		*Term,
		*FString::Join(Errors.Errors, TEXT("\n"))
	));
	return false;
}

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
		VTC_MUST_EQUAL(FVulField::Create(&Root).SerializeToJson(JsonStr), true, "serialize")
		
		TC.Equal(
			*JsonStr,
			TEXT("{\"type\":\"Base\",\"children\":[{\"type\":\"Node2\",\"children\":[{\"type\":\"Node1\",\"int\":-5}],\"str\":\"foo\"},{\"type\":\"Node1\",\"int\":13}]}"),
			"Json is equal"
		);
		
	
		// Deserialize it back in to an empty tree struct.
		TSharedPtr<FVulFieldTestTreeBase> DeserializedRoot = MakeShared<FVulFieldTestTreeBase>();
	
		FVulFieldDeserializationContext Ctx;
		VTC_MUST_EQUAL(FVulField::Create(&DeserializedRoot).DeserializeFromJson(JsonStr, Ctx), true, "deserialize")
	
		// Serialize it again as an easy way to assert the structure is correct.
		FString JsonStr2;
		VTC_MUST_EQUAL(FVulField::Create(&Root).SerializeToJson(JsonStr2), true, "serialize again")
	
		TC.Equal(*JsonStr2, *JsonStr, "serialize");
	});
	
	VulTest::Case(this, "Deserialize references: TSharedPtr", [](VulTest::TC TC)
	{
		TSharedPtr<FVulFieldTestSingleInstance> Ptr1;
		TSharedPtr<FVulFieldTestSingleInstance> Ptr2;
		
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Ptr1), "instance1");
		Set.Add(FVulField::Create(&Ptr2), "instance2");
	
		const auto Json = TEXT("{\"instance1\":{\"int\":5,\"str\":\"foobar\"},\"instance2\":\"foobar\"}");
	
		VTC_MUST_EQUAL(Set.DeserializeFromJson(Json), true, "deserialize")
	
		TC.Equal(Ptr1.Get(), Ptr2.Get(), "pointers same");
	});
	
	VulTest::Case(this, "Deserialize references: raw pointers", [](VulTest::TC TC)
	{
		// Test with an array for more coverage of recursive TVulFieldSerializer interpretation.
		TArray<FVulFieldTestSingleInstance*> Arr = {};
		
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Arr), "data");
	
		const auto Json = TEXT("{\"data\":[{\"int\":5,\"str\":\"foobar\"},\"foobar\"]}");
	
		FVulFieldDeserializationContext Ctx;
		VTC_MUST_EQUAL(Set.DeserializeFromJson(Json, Ctx), true, "deserialize")
	
		if (TC.Equal(Arr.Num(), 2, "pointer array length"))
		{
			TC.Equal(Arr[0], Arr[1], "pointers same");
		}
	});
	
	VulTest::Case(this, "Deserialize references: instances", [](VulTest::TC TC)
	{
		// When using references in the serialized form to non-pointer variables, we
		// get different instances, but the same data.
		FVulFieldTestSingleInstance Instance1;
		FVulFieldTestSingleInstance Instance2;
		
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Instance1), "instance1");
		Set.Add(FVulField::Create(&Instance2), "instance2");
	
		const auto Json = TEXT("{\"instance1\":{\"int\":5,\"str\":\"foobar\"},\"instance2\":\"foobar\"}");
	
		VTC_MUST_EQUAL(Set.DeserializeFromJson(Json), true, "deserialize")
	
		TC.Equal(Instance1.Int, Instance2.Int, "int same");
		TC.Equal(Instance1.Str, Instance2.Str, "str same");
	});
	
	VulTest::Case(this, "UObject", [](VulTest::TC TC)
	{
		UObject* Outer = NewObject<AActor>();
		
		UVulFieldTestUObject1* TestObj1 = nullptr;
	
		FString JsonStr = "{\"str\":\"foobar\",\"obj\":{\"str\":\"qux\"}}";
	
		FVulFieldDeserializationContext Ctx;
		Ctx.ObjectOuter = Outer;
		VTC_MUST_EQUAL(FVulField::Create(&TestObj1).DeserializeFromJson(JsonStr, Ctx), true, "deserialize obj1")
	
		VTC_MUST_EQUAL(*TestObj1->Str, TEXT("foobar"), "deserialize obj1: str correct")
		VTC_MUST_EQUAL(TestObj1->GetOuter(), Outer, "deserialize obj1: outer correct")
		VTC_MUST_EQUAL(IsValid(TestObj1->Obj), true, "deserialize obj1: nested object is valid")
		VTC_MUST_EQUAL(*TestObj1->Obj->Str, TEXT("qux"), "deserialize obj1: nested object is correct")
		VTC_MUST_EQUAL(TestObj1->Obj->GetOuter(), Outer, "deserialize obj1: nested object outer correct")
	
		FString SerializedJson;
		VTC_MUST_EQUAL(FVulField::Create(&TestObj1).SerializeToJson(SerializedJson), true, "serialize obj1")
		
		VTC_MUST_EQUAL(*SerializedJson, *JsonStr, "serialize obj1")
	
		TMap<FString, UVulFieldTestUObject1*> Map;
		FString MapJsonStr = TEXT("{\"obj1\":{\"str\":\"foobar\",\"obj\":{\"str\":\"qux\"}},\"obj2\":\"foobar\"}");
		VTC_MUST_EQUAL(FVulField::Create(&Map).DeserializeFromJson(MapJsonStr, Ctx), true, "deserialize map")
	
		VTC_MUST_EQUAL(Map.Num(), 2, "deserialize map: num correct")
		VTC_MUST_EQUAL(Map.Contains("obj1"), true, "deserialize map: contains obj1")
		VTC_MUST_EQUAL(Map.Contains("obj2"), true, "deserialize map: contains obj2")
		VTC_MUST_EQUAL(Map["obj1"], Map["obj2"], "deserialize map: same object")
	
		VTC_MUST_EQUAL(FVulField::Create(&Map).SerializeToJson(SerializedJson), true, "serialize map")
		VTC_MUST_EQUAL(*SerializedJson, *MapJsonStr, "serialize map correctly")
	});
	
	VulTest::Case(this, "UObject - assets", [](VulTest::TC TC)
	{
		// This test requires an asset, assuming this texture exists in engine content.
		UTexture2D* Texture = LoadObject<UTexture2D>(
			nullptr,
			TEXT("Texture2D'/Engine/EngineSky/T_Sky_Blue.T_Sky_Blue'")
		);

		VTC_MUST_EQUAL(IsValid(Texture), true, "loaded engine content")

		// Need a container as UE JSON doesn't do scalar roots.
		TArray Textures = {Texture};

		FString SerializedJson;
		
		{
			FVulFieldSerializationContext Ctx;
			VTC_MUST_EQUAL(FVulField::Create(&Textures).SerializeToJson(SerializedJson, Ctx), true, "serialize")
			VTC_MUST_EQUAL(*SerializedJson, TEXT("[\"/Engine/EngineSky/T_Sky_Blue.T_Sky_Blue\"]"), "serialize correctly");
		}

		{
			TArray<UTexture2D*> DeserializedTextures = {};
			FVulFieldDeserializationContext Ctx;
			VTC_MUST_EQUAL(FVulField::Create(&DeserializedTextures).DeserializeFromJson(SerializedJson, Ctx), true, "deserialize")
			VTC_MUST_EQUAL(DeserializedTextures, Textures, "deserialize correctly");
		}
	});

	VulTest::Case(this, "TScriptInterface - valid interface", [](VulTest::TC TC)
	{
		FVulFieldDeserializationContext Ctx;
		UObject* Outer = NewObject<AActor>();
		Ctx.ObjectOuter = Outer;
		
		TArray<TScriptInterface<IVulFieldTestInterface1>> Interfaces;
		
		UVulFieldTestUObject2* TestObj = nullptr;
		FVulFieldSet FieldSet;
		FieldSet.Add(FVulField::Create(&TestObj), "obj");
		FieldSet.Add(FVulField::Create(&Interfaces), "interfaces");

		FString JsonStr = "{\"obj\":{\"str\":\"qux\"},\"interfaces\":[\"qux\",\"qux\",\"qux\"]}";
		VTC_MUST_EQUAL(FieldSet.DeserializeFromJson(JsonStr, Ctx), true, "deserialize field set")
		VTC_MUST_EQUAL(IsValid(TestObj), true, "object is valid")
		VTC_MUST_EQUAL(Interfaces.Num(), 3, "interfaces num")
		VTC_MUST_EQUAL(static_cast<void*>(TestObj), static_cast<void*>(Interfaces[0].GetObject()), "interfaces[0] is same")
		VTC_MUST_EQUAL(static_cast<void*>(TestObj), static_cast<void*>(Interfaces[1].GetObject()), "interfaces[1] is same")
		VTC_MUST_EQUAL(static_cast<void*>(TestObj), static_cast<void*>(Interfaces[2].GetObject()), "interfaces[2] is same")
	});

	VulTest::Case(this, "TScriptInterface - invalid interface", [](VulTest::TC TC)
	{
		FVulFieldDeserializationContext Ctx;
		UObject* Outer = NewObject<AActor>();
		Ctx.ObjectOuter = Outer;
		
		TArray<TScriptInterface<UVulFieldTestInterface2>> Interfaces;
		
		UVulFieldTestUObject2* TestObj = nullptr;
		FVulFieldSet FieldSet;
		FieldSet.Add(FVulField::Create(&TestObj), "obj");
		FieldSet.Add(FVulField::Create(&Interfaces), "interfaces");

		FString JsonStr = "{\"obj\":{\"str\":\"qux\"},\"interfaces\":[\"qux\",\"qux\",\"qux\"]}";
		VTC_MUST_EQUAL(FieldSet.DeserializeFromJson(JsonStr, Ctx), false, "deserialize failed")

		CtxContainsError(TC, Ctx.Errors, "deserialized object of class which does not implement the expected interface");
	});

	VulTest::Case(this, "Test enum", [](VulTest::TC TC)
	{
		// Note we have to use a container as UE JSON deserialization does not support scalar roots.
		TArray<EVulFieldTestTreeNodeType> EnumValues;

		FString JsonStr = "[\"node1\"]";
		FVulFieldDeserializationContext Ctx;
		
		VTC_MUST_EQUAL(FVulField::Create(&EnumValues).DeserializeFromJson(JsonStr, Ctx), true, "deserialize")

		VTC_MUST_EQUAL(EnumValues, {EVulFieldTestTreeNodeType::Node1}, "enum is correct")

		FString SerializedStr;
		VTC_MUST_EQUAL(FVulField::Create(&EnumValues).SerializeToJson(SerializedStr), true, "serialize")

		VTC_MUST_EQUAL(*SerializedStr, *JsonStr, "serialized correctly")
	});

	VulTest::Case(this, "Test TPair", [](VulTest::TC TC)
	{
		TPair<FString, int> Pair = {"foo", 13};

		FVulFieldDeserializationContext Ctx;

		FString SerializedStr;
		VTC_MUST_EQUAL(FVulField::Create(&Pair).SerializeToJson(SerializedStr), true, "serialize")
		VTC_MUST_EQUAL(*SerializedStr, TEXT("[\"foo\",13]"), "serialized correctly");
		
		TPair<FString, int> DeserializedPair;
		VTC_MUST_EQUAL(FVulField::Create(&DeserializedPair).DeserializeFromJson(SerializedStr, Ctx), true, "deserialize")
		VTC_MUST_EQUAL(*DeserializedPair.Key, TEXT("foo"), "deserialized correctly: key")
		VTC_MUST_EQUAL(DeserializedPair.Value, 13, "deserialized correctly: value")
	});

	VulTest::Case(this, "Test Float", [](VulTest::TC TC)
	{
		TArray<float> Floats = {1.2, 2.1, 3.5, 5.3};

		FVulFieldDeserializationContext Ctx;

		FString SerializedStr;
		VTC_MUST_EQUAL(FVulField::Create(&Floats).SerializeToJson(SerializedStr), true, "serialize")
		VTC_MUST_EQUAL(*SerializedStr, TEXT("[1.2,2.1,3.5,5.3]"), "serialized correctly");
		
		TArray<float> DeserializedFloats;
		VTC_MUST_EQUAL(FVulField::Create(&DeserializedFloats).DeserializeFromJson(SerializedStr, Ctx), true, "deserialize")
		VTC_MUST_EQUAL(DeserializedFloats, Floats, "deserialized correctly");
	});

	VulTest::Case(this, "Test single field type", [](VulTest::TC TC)
	{
		TArray<FVulSingleFieldType> SingleFields;

		SingleFields.AddDefaulted(3);
		SingleFields[0].Value = 5;
		SingleFields[1].Value = -5;
		SingleFields[2].Value = -20;

		FVulFieldDeserializationContext Ctx;

		FString SerializedStr;
		VTC_MUST_EQUAL(FVulField::Create(&SingleFields).SerializeToJson(SerializedStr), true, "serialize")
		VTC_MUST_EQUAL(*SerializedStr, TEXT("[5,-5,-20]"), "serialized correctly");
		
		TArray<FVulSingleFieldType> DeserializedSingleFields;
		VTC_MUST_EQUAL(FVulField::Create(&DeserializedSingleFields).DeserializeFromJson(SerializedStr, Ctx), true, "deserialize")
		VTC_MUST_EQUAL(DeserializedSingleFields.Num(), 3, "deserialized correctly: num");
		VTC_MUST_EQUAL(DeserializedSingleFields[0].Value, 5, "deserialized correctly: 0");
		VTC_MUST_EQUAL(DeserializedSingleFields[1].Value, -5, "deserialized correctly: 1");
		VTC_MUST_EQUAL(DeserializedSingleFields[2].Value, -20, "deserialized correctly: 2");
	});

	{
		struct Data
		{
			TArray<FString> Array;
			TMap<FString, TArray<FString>> Map;
			TSharedPtr<FString> Ptr;
			TOptional<FString> Optional;
			FString Str;
			FString ExpectedJson;
			bool EvenIfEmpty = false;
		};
		
		auto Ddt = VulTest::DDT<Data>(this, "omit empty serialization", [](const VulTest::TestCase& TC, const Data& Data)
		{
			FVulFieldSet Set;
			Set.Add(FVulField::Create(&Data.Array), "array").EvenIfEmpty(Data.EvenIfEmpty);
			Set.Add(FVulField::Create(&Data.Map), "map").EvenIfEmpty(Data.EvenIfEmpty);
			Set.Add(FVulField::Create(&Data.Ptr), "ptr").EvenIfEmpty(Data.EvenIfEmpty);
			Set.Add(FVulField::Create(&Data.Str), "str").EvenIfEmpty(Data.EvenIfEmpty);
			Set.Add(FVulField::Create(&Data.Optional), "optional").EvenIfEmpty(Data.EvenIfEmpty);

			FString ActualJson;
			FVulFieldSerializationContext Ctx;
			
			VTC_MUST_EQUAL(Set.SerializeToJson(ActualJson, Ctx), true, "serialization succeeds")
			VTC_MUST_EQUAL(*ActualJson, *Data.ExpectedJson, "json equal");
		});

		Ddt.Run("all empty - include", {
			.Array = {},
			.Map = {},
			.Ptr = nullptr,
			.Optional = {},
			.Str = "",
			.ExpectedJson = "{\"array\":[],\"map\":{},\"ptr\":null,\"str\":\"\",\"optional\":null}",
			.EvenIfEmpty = true,
		});

		Ddt.Run("all empty - omit", {
			.Array = {},
			.Map = {},
			.Ptr = nullptr,
			.Optional = {},
			.Str = "",
			.ExpectedJson = "{}",
			.EvenIfEmpty = false,
		});

		Ddt.Run("complex empty - omit", {
			.Array = {"", "", ""},
			.Map = {{"foo", {}}, {"bar", {""}}},
			.Ptr = MakeShared<FString>(""),
			.Optional = FString(""),
			.Str = "",
			.ExpectedJson = "{}",
			.EvenIfEmpty = false,
		});
	}

	{
		struct Data
		{
			FString Json;
			mutable FVulField Field;
			TArray<FString> ExpectedErrors;
		};
		
		auto Ddt = VulTest::DDT<Data>(this, "error messages: deserialization", [](const VulTest::TestCase& TC, const Data& Data)
		{
			FVulFieldDeserializationContext Ctx;
			
			VTC_MUST_EQUAL(Data.Field.DeserializeFromJson(Data.Json, Ctx), false, "trees fails")
			
			for (const auto Err : Data.ExpectedErrors)
			{
				CtxContainsError(TC, Ctx.Errors, Err);
			}
		});
		
		TArray<TSharedPtr<FVulFieldTestTreeBase>> Trees;

		Ddt.Run("trees 1", Data{
			.Json = R"(
				[
				  {
				    "type": "node1",
				    "int": "not an int",
				    "children": []
				  }
				]
			)",
			.Field = FVulField::Create(&Trees),
			.ExpectedErrors = {
				".[0].int: Required JSON type Number, but got String",
			},
		});

		Ddt.Run("trees 2", Data{
			.Json = R"(
				[
				  {
				    "type": "node1",
				    "int": 13,
				    "children": []
				  },
				  {
				    "type": "node1",
				    "int": "not an int",
				    "children": []
				  }
				]
			)",
			.Field = FVulField::Create(&Trees),
			.ExpectedErrors = {
				".[1].int: Required JSON type Number, but got String",
			},
		});

		Ddt.Run("trees 3", Data{
			.Json = R"(
				[
				  {
				    "type": "node1",
				    "int": 13,
				    "children": []
				  },
				  {
				    "type": "node1",
				    "int": 14,
				    "children": [
				      {
				        "type": "node2",
				        "str": "a string",
				        "children": [
				          {
				            
				          }
				        ]
				      }
				    ]
				  }
				]
			)",
			.Field = FVulField::Create(&Trees),
			.ExpectedErrors = {
				".[1].children.[0].children.[0]: Required JSON property `type` is not defined",
			},
		});
	}
	
	return true;
}
