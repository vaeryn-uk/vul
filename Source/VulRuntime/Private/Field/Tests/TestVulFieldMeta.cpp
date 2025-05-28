#include "TestCase.h"
#include "TestVulFieldStructs.h"
#include "Misc/AutomationTest.h"
#include "Misc/VulNumber.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestVulFieldMeta,
	"VulRuntime.Field.TestVulFieldMeta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

template <typename T>
bool TestDescribe(VulTest::TC TC, FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Desc)
{
	const auto Ok = Ctx.Describe<T>(Desc);

	if (!Ok)
	{
		TC.Error("Describe() failed");
		Ctx.State.Errors.Log();
	}

	return Ok;
}

bool TestDescribe(VulTest::TC TC, const FVulFieldSet& Set, FVulFieldSerializationContext& Ctx, TSharedPtr<FVulFieldDescription>& Desc)
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
  "$ref": "#definitions/VulFieldTestTreeNodeType",
  "definitions": {
    "VulFieldTestTreeNodeType": {
      "type": "string",
      "enum": ["Base", "Node1", "Node2"],
      "x-vul-typename": "VulFieldTestTreeNodeType"
    }
  }
})";
		
		(void)TC.JsonObjectsEqual(JsonSchema, Expected);
	});
	
	VulTest::Case(this, "Schema generation - inheritance tree", [](VulTest::TC TC)
	{
		TSharedPtr<FVulFieldTestTreeBase> Base;
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Base), "base");
		
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, TestDescribe(TC, Set, Ctx, Desc), "");

		const auto JsonSchema = VulRuntime::Field::JsonToString(Desc->JsonSchema());

		FString Expected = R"(
{
  "type": "object",
  "properties": {
    "base": {
      "$ref": "#definitions/VulFieldTestTreeBase"
    }
  },
  "definitions": {
    "VulFieldTestTreeBase": {
      "type": [ "object", "null" ],
      "properties": {
        "type": { "$ref": "#definitions/VulFieldTestTreeNodeType" },
        "children": {
          "type": "array",
          "items": { "$ref": "#definitions/VulFieldTestTreeBase" }
        }
      },
      "oneOf": [
        {
          "$ref": "#definitions/VulFieldTestTreeNode1"
        },
        {
          "$ref": "#definitions/VulFieldTestTreeNode2"
        }
      ],
      "x-vul-typename": "VulFieldTestTreeBase"
    },
    "VulFieldTestTreeNodeType": {
      "type": "string",
      "enum": ["Base", "Node1", "Node2"],
      "x-vul-typename": "VulFieldTestTreeNodeType"
    },
    "VulFieldTestTreeNode1": {
      "type": "object",
      "properties": {
        "type": {
          "const": "Node1"
        },
        "children": {
          "type": "array",
          "items": {
            "$ref": "#definitions/VulFieldTestTreeBase"
          }
        },
        "int": {
          "type": "number"
        }
      },
      "required": [
        "type"
      ],
      "x-vul-typename": "VulFieldTestTreeNode1"
    },
    "VulFieldTestTreeNode2": {
      "type": "object",
      "properties": {
        "type": {
          "const": "Node2"
        },
        "children": {
          "type": "array",
          "items": {
            "$ref": "#definitions/VulFieldTestTreeBase"
          }
        },
        "str": {
          "type": "string"
        }
      },
      "required": [
        "type"
      ],
      "x-vul-typename": "VulFieldTestTreeNode2"
    }
  }
}
)";
		
		(void)TC.JsonObjectsEqual(JsonSchema, Expected);
	});
	
	VulTest::Case(this, "Typescript definitions", [](VulTest::TC TC)
	{
		TSharedPtr<FVulFieldTestTreeBase> Base;
		FMyStringAlias StrAlias;
		UVulFieldTestUObject1* Obj;
		FVulSingleFieldType SingleFieldType;
		
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Base), "base");
		Set.Add(FVulField::Create(&StrAlias), "strAlias");
		Set.Add(FVulField::Create(&Obj), "uObject");
		Set.Add(FVulField::Create(&SingleFieldType), "singleField");
		
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, TestDescribe(TC, Set, Ctx, Desc), "");

		FString Expected = R"(
// A string reference to an existing object of the given type
// @ts-ignore
export type VulFieldRef<T> = string;

export interface VulFieldTestTreeBase {
	type?: VulFieldTestTreeNodeType;
	children?: VulFieldTestTreeBase[];
}

export enum VulFieldTestTreeNodeType {
	Base = "Base",
	Node1 = "Node1",
	Node2 = "Node2",
}

export interface VulFieldTestTreeNode1 extends VulFieldTestTreeBase {
	type?: VulFieldTestTreeNodeType.Node1;
	int?: number;
}

export interface VulFieldTestTreeNode2 extends VulFieldTestTreeBase {
	type?: VulFieldTestTreeNodeType.Node2;
	str?: string;
}

export type StringAlias = string;

export interface VulFieldTestUObject1 {
	str?: string;
	obj?: (VulFieldTestUObject2 | VulFieldRef<VulFieldTestUObject2>);
}

export interface VulFieldTestUObject2 {
	str?: string;
}

export type SingleFieldType = number;
)";

		const auto Actual = Desc->TypeScriptDefinitions();

		if (!TC.EqualNoWhitespace(Actual, Expected, "typescript definition match"))
		{
			return;
		}
	});
	
	VulTest::Case(this, "Typescript definitions - UINTERFACES", [](VulTest::TC TC)
	{
		TSharedPtr<FVulFieldTestTreeBase> Base;

		TScriptInterface<IVulFieldTestInterface1> Interface;
		
		FVulFieldSet Set;
		Set.Add(FVulField::Create(&Interface), "uInterface").EvenIfEmpty();
		
		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, TestDescribe(TC, Set, Ctx, Desc), "");

		FString Expected = R"(
// A string reference to an existing object of the given type
// @ts-ignore
export type VulFieldRef<T> = string;

export interface IVulFieldTestInterface1 {
}

export interface VulFieldTestUObject2 extends IVulFieldTestInterface1 {
	str?: string;
}

export interface VulFieldTestUObject3 extends IVulFieldTestInterface1 {
	bool?: boolean;
}
)";

		const auto Actual = Desc->TypeScriptDefinitions();

		if (!TC.EqualNoWhitespace(Actual, Expected, "typescript definition match"))
		{
			return;
		}
	});
	
	VulTest::Case(this, "Referencing", [](VulTest::TC TC)
	{
		UVulTestFieldReferencing* UVulTestFieldReferencing = nullptr;
		UVulTestFieldReferencingContainer1* UVulTestFieldReferencingContainer1 = nullptr;
		UVulTestFieldReferencingContainer2* UVulTestFieldReferencingContainer2 = nullptr;

		FVulFieldSet Set;
		Set.Add(FVulField::Create(UVulTestFieldReferencing), "UVulTestFieldReferencing").EvenIfEmpty();
		Set.Add(FVulField::Create(UVulTestFieldReferencingContainer1), "UVulTestFieldReferencingContainer1").EvenIfEmpty();
		Set.Add(FVulField::Create(UVulTestFieldReferencingContainer2), "UVulTestFieldReferencingContainer2").EvenIfEmpty();

		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		Ctx.Flags.Set(VulFieldSerializationFlag_Referencing, false, ".UVulTestFieldReferencingContainer2");
		VTC_MUST_EQUAL(true, TestDescribe(TC, Set, Ctx, Desc), "");

		{ // json schema
			const auto Actual = VulRuntime::Field::JsonToString(Desc->JsonSchema());

			const auto Expected = R"(
{
  "type": "object",
  "properties": {
    "UVulTestFieldReferencing": {
      "oneOf": [
        {
          "$ref": "#definitions/VulTestFieldReferencing"
        },
        {
          "$ref": "#definitions/VulFieldRef"
        }
      ]
    },
    "UVulTestFieldReferencingContainer1": {
      "$ref": "#definitions/VulTestFieldReferencingContainer1"
    },
    "UVulTestFieldReferencingContainer2": {
      "$ref": "#definitions/VulTestFieldReferencingContainer2"
    }
  },
  "required": [
    "UVulTestFieldReferencing",
    "UVulTestFieldReferencingContainer1",
    "UVulTestFieldReferencingContainer2"
  ],
  "definitions": {
    "VulTestFieldReferencing": {
      "type": "object",
      "properties": {
        "name": {
          "type": "string"
        }
      },
      "x-vul-typename": "VulTestFieldReferencing"
    },
    "VulTestFieldReferencingContainer1": {
      "type": "object",
      "properties": {
        "child": {
          "oneOf": [
            {
              "$ref": "#definitions/VulTestFieldReferencing"
            },
            {
              "$ref": "#definitions/VulFieldRef"
            }
          ]
        }
      },
      "x-vul-typename": "VulTestFieldReferencingContainer1"
    },
    "VulTestFieldReferencingContainer2": {
      "type": "object",
      "properties": {
        "child": {
          "oneOf": [
            {
              "$ref": "#definitions/VulTestFieldReferencing"
            },
            {
              "$ref": "#definitions/VulFieldRef"
            }
          ]
        }
      },
      "x-vul-typename": "VulTestFieldReferencingContainer2"
    },
    "VulFieldRef": {
      "type": "string",
      "description": "A string reference to another object in the graph."
    }
  }
}
)";

			if (!TC.JsonObjectsEqual(Actual, Expected, "json schemas match"))
			{
				return;
			}
		}

		{ // typescript.
			const auto Expected = R"(
// A string reference to an existing object of the given type
// @ts-ignore
export type VulFieldRef<T> = string;

export interface VulTestFieldReferencing {
	name?: string;
}

export interface VulTestFieldReferencingContainer1 {
	child?: (VulTestFieldReferencing | VulFieldRef<VulTestFieldReferencing>);
}

export interface VulTestFieldReferencingContainer2 {
	child?: (VulTestFieldReferencing | VulFieldRef<VulTestFieldReferencing>);
}
)";

			const auto Actual = Desc->TypeScriptDefinitions();

			if (!TC.EqualNoWhitespace(Actual, Expected, "typescript definition match"))
			{
				return;
			}
		}
	});

	VulTest::Case(this, "Extracted referencing", [](VulTest::TC TC)
	{
		UVulTestFieldReferencing* UVulTestFieldReferencing = nullptr;
		UVulTestFieldReferencingContainer1* UVulTestFieldReferencingContainer1 = nullptr;
		UVulTestFieldReferencingContainer2* UVulTestFieldReferencingContainer2 = nullptr;

		FVulFieldSet Set;
		Set.Add(FVulField::Create(UVulTestFieldReferencing), "UVulTestFieldReferencing").EvenIfEmpty();
		Set.Add(FVulField::Create(UVulTestFieldReferencingContainer1), "UVulTestFieldReferencingContainer1").EvenIfEmpty();
		Set.Add(FVulField::Create(UVulTestFieldReferencingContainer2), "UVulTestFieldReferencingContainer2").EvenIfEmpty();

		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		Ctx.Flags.Set(VulFieldSerializationFlag_Referencing, false, ".UVulTestFieldReferencingContainer2");
		Ctx.ExtractReferences = true;
		VTC_MUST_EQUAL(true, TestDescribe(TC, Set, Ctx, Desc), "");

		{ // json schema
			const auto Actual = VulRuntime::Field::JsonToString(Desc->JsonSchema());

			const auto Expected = R"(
{
  "type": "object",
  "properties": {
    "refs": {
      "type": "object"
    },
    "data": {
      "type": "object",
      "properties": {
        "UVulTestFieldReferencing": {
          "$ref": "#definitions/VulFieldRef"
        },
        "UVulTestFieldReferencingContainer1": {
          "$ref": "#definitions/VulTestFieldReferencingContainer1"
        },
        "UVulTestFieldReferencingContainer2": {
          "$ref": "#definitions/VulTestFieldReferencingContainer2"
        }
      },
      "required": [
        "UVulTestFieldReferencing",
        "UVulTestFieldReferencingContainer1",
        "UVulTestFieldReferencingContainer2"
      ]
    }
  },
  "definitions": {
    "VulTestFieldReferencing": {
      "type": "object",
      "properties": {
        "name": {
          "type": "string"
        }
      },
      "x-vul-typename": "VulTestFieldReferencing"
    },
    "VulTestFieldReferencingContainer1": {
      "type": "object",
      "properties": {
        "child": {
          "$ref": "#definitions/VulFieldRef"
        }
      },
      "x-vul-typename": "VulTestFieldReferencingContainer1"
    },
    "VulTestFieldReferencingContainer2": {
      "type": "object",
      "properties": {
        "child": {
          "$ref": "#definitions/VulFieldRef"
        }
      },
      "x-vul-typename": "VulTestFieldReferencingContainer2"
    },
    "VulFieldRef": {
      "type": "string",
      "description": "A string reference to another object in the graph."
    }
  }
}
)";

			if (!TC.JsonObjectsEqual(Actual, Expected, "json schemas match"))
			{
				return;
			}
		}

		{ // typescript.
			const auto Expected = R"(
// A string reference to an existing object of the given type
// @ts-ignore
export type VulFieldRef<T> = string;

export type VulRefs = Record<VulFieldRef<any>, any>;

export interface VulTestFieldReferencing {
    name?: string;
}

export interface VulTestFieldReferencingContainer1 {
    child?: VulFieldRef<VulTestFieldReferencing>;
}

export interface VulTestFieldReferencingContainer2 {
    child?: VulFieldRef<VulTestFieldReferencing>;
}
)";

			const auto Actual = Desc->TypeScriptDefinitions();

			if (!TC.EqualNoWhitespace(Actual, Expected, "typescript definition match"))
			{
				return;
			}
		}
	});
	
	VulTest::Case(this, "Typescript definitions - TVulNumber", [](VulTest::TC TC)
	{
		using FVulTestNumber = TVulNumber<int>;

		FVulFieldSerializationContext Ctx;
		TSharedPtr<FVulFieldDescription> Desc = MakeShared<FVulFieldDescription>();
		VTC_MUST_EQUAL(true, TestDescribe<FVulTestNumber>(TC, Ctx, Desc), "");

		const auto Expected = R"(
export interface VulNumber {
	base?: number;
	clamp?: VulNumber[];
	modifications?: VulNumberModification[];
	value?: number;
}

export interface VulNumberModification {
	clamp?: number[];
	pct?: number;
	basePct?: number;
	flat?: number;
	set?: number;
	id?: string;
}
)";

		const auto Actual = Desc->TypeScriptDefinitions();

		if (!TC.EqualNoWhitespace(Actual, Expected, "typescript definition match"))
		{
			return;
		}
	});
	
	return true;
}
