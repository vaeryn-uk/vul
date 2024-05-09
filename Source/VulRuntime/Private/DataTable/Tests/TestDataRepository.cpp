#include "TestCase.h"
#include "TestVulDataStructs.h"
#include "DataTable/VulDataRepository.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestDataRepository,
	"VulRuntime.DataTable.TestDataRepository",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestDataRepository::RunTest(const FString& Parameters)
{
	{
		auto DT1 = NewObject<UDataTable>();
		DT1->RowStruct = FTestTableRow1::StaticStruct();
		auto DT2 = NewObject<UDataTable>();
		DT2->RowStruct = FTestTableRow2::StaticStruct();

		DT1->AddRow(FName("Table1Row1"), FTestTableRow1(10));

		auto TTR2 = FTestTableRow2();
		TTR2.ARef = FVulDataPtr("Table1Row1");
		DT2->AddRow(FName("Table2Row1"), TTR2);

		auto Repo = NewObject<UVulDataRepository>();

		Repo->DataTables = {
			{FName("T1"), DT1},
			{FName("T2"), DT2},
		};

		{
			auto Row = Repo->FindChecked<FTestTableRow1>(FName("T1"), FName("Table1Row1"));
			TestEqual("Repository FindPtrChecked", Row.Get()->Value, 10);
		}

		{
			auto Row = Repo->FindChecked<FTestTableRow2>(FName("T2"), FName("Table2Row1"));
			TestEqual("Repository FindPtrChecked", Row.Get()->ARef.Get<FTestTableRow1>()->Value, 10);
		}
	}

	{
		auto DT = NewObject<UDataTable>();
		DT->RowStruct = FCircDep::StaticStruct();

		auto Child1 = FCircDep(20);
		auto Child2 = FCircDep(30);
		auto Child3 = FCircDep(40);

		DT->AddRow(FName("ChildRow1"), Child1);
		DT->AddRow(FName("ChildRow2"), Child2);
		DT->AddRow(FName("ChildRow3"), Child3);

		auto Parent = FCircDep(10);
		Parent.CircularProperty = FVulDataPtr("ChildRow1");

		Parent.CircularArray.Add(FVulDataPtr("ChildRow1"));
		Parent.CircularArray.Add(FVulDataPtr("ChildRow2"));

		Parent.CircularMap.Add("a", FCircDepIncluder(15, FVulDataPtr("ChildRow2")));
		Parent.CircularMap.Add("b", FCircDepIncluder(25, FVulDataPtr("ChildRow3")));

		DT->AddRow(FName("ParentRow"), Parent);

		auto Repo = NewObject<UVulDataRepository>();

		Repo->DataTables = {
			{FName("CircTable"), DT},
		};

		auto FoundParent = Repo->FindChecked<FCircDep>(FName("CircTable"), FName("ParentRow"));
		TestEqual("CircDep parent", FoundParent.Get()->Value, 10);
		TestEqual("CircDep property", FoundParent.Get()->CircularProperty.Get<FCircDep>()->Value, 20);
		if (TestEqual("CircDep array length", FoundParent.Get()->CircularArray.Num(), 2))
		{
			TestEqual("CircDep array[0]", FoundParent.Get()->CircularArray[0].Get<FCircDep>()->Value, 20);
			TestEqual("CircDep array[1]", FoundParent.Get()->CircularArray[1].Get<FCircDep>()->Value, 30);
		}

		if (TestEqual("CircDep map length", FoundParent.Get()->CircularMap.Num(), 2))
		{
			if (TestTrue("CircDep map[\"a\"]", FoundParent.Get()->CircularMap.Contains("a")))
			{
				TestEqual("CircDep map[\"a\"] value", FoundParent.Get()->CircularMap["a"].SomeValue, 15);
				TestEqual("CircDep map[\"a\"] ref", FoundParent.Get()->CircularMap["a"].CircularProperty.Get<FCircDep>()->Value, 30);
			}

			if (TestTrue("CircDep map[\"b\"]", FoundParent.Get()->CircularMap.Contains("b")))
			{
				TestEqual("CircDep map[\"b\"] value", FoundParent.Get()->CircularMap["b"].SomeValue, 25);
				TestEqual("CircDep map[\"b\"] ref", FoundParent.Get()->CircularMap["b"].CircularProperty.Get<FCircDep>()->Value, 40);
			}
		}
	}

	{
		auto Child1Table = NewObject<UDataTable>();
		Child1Table->RowStruct = FVulTestChild1Struct::StaticStruct();

		FVulTestChild1Struct Child1Struct;
		Child1Struct.Child1Field = "child 1 field";
		Child1Struct.ParentField = "parent 1 field";

		Child1Table->AddRow(FName("child1struct"), Child1Struct);

		auto Child2Table = NewObject<UDataTable>();
		Child2Table->RowStruct = FVulTestChild2Struct::StaticStruct();

		FVulTestChild2Struct Child2Struct;
		Child2Struct.Child2Field = "child 2 field";
		Child2Struct.ParentField = "parent 2 field";

		Child2Table->AddRow(FName("child2struct"), Child2Struct);

		auto Repo = NewObject<UVulDataRepository>();
		Repo->DataTables.Add("child1", Child1Table);
		Repo->DataTables.Add("child2", Child2Table);

		auto FoundChild1 = Repo->FindChecked<FVulTestChild1Struct>("child1", "child1struct");

		TestEqual("child1ptr: parent field", FoundChild1.Get()->ParentField, "parent 1 field");
		TestEqual("child1ptr: child field", FoundChild1.Get()->Child1Field, "child 1 field");

		// Up-cast.
		auto Child1AsParent = Cast<FVulTestBaseStruct>(FoundChild1);
		TestEqual("child1ptr as parent: parent field", Child1AsParent.Get()->ParentField, "parent 1 field");

		// Down-cast.
		auto Child1AsChild1 = Cast<FVulTestChild1Struct>(Child1AsParent);
		TestEqual("child1ptr as child: parent field", Child1AsChild1.Get()->ParentField, "parent 1 field");
		TestEqual("child1ptr as child: parent field", Child1AsChild1.Get()->Child1Field, "child 1 field");

		// Failed downcast.
		TestFalse("downcast fails cleanly", Cast<FVulTestChild2Struct>(Child1AsParent).IsSet());

		// Implicit casts.
		Child1AsParent = FoundChild1;
		TestEqual("implicit up cast", Child1AsParent.Get()->ParentField, "parent 1 field");
		Child1AsChild1 = Child1AsParent;
		TestEqual("implicit down cast", Child1AsParent.Get()->ParentField, "parent 1 field");
	}

	VulTest::Case(this, "indirect referencing", [](VulTest::TC TC) {
		// Tests that references on embedded objects are traversed & resolved.

		auto RowTable = NewObject<UDataTable>();
		RowTable->RowStruct = FTestTableRow1::StaticStruct();

		FTestTableRow1 Data;
		Data.Value = 13;
		RowTable->AddRow(FName("data"), Data);

		auto ReferencingTable = NewObject<UDataTable>();
		ReferencingTable->RowStruct = FVulIndirectRefParent::StaticStruct();

		FVulDirectRef Direct;
		Direct.Data = FVulDataPtr("data");

		FVulIndirectRefParent Parent;
		Parent.Property = Direct;
		Parent.Array = {Direct, Direct, Direct};
		Parent.Map = {{1, Direct}, {2, Direct}, {3, Direct}};

		ReferencingTable->AddRow("parent", Parent);

		auto Repo = NewObject<UVulDataRepository>();
		Repo->DataTables.Add("RowTable", RowTable);
		Repo->DataTables.Add("Referencing", ReferencingTable);

		const auto Found = Repo->FindChecked<FVulIndirectRefParent>("Referencing", "parent");

		TC.Equal(Found.Get()->Property.Data.Get<FTestTableRow1>()->Value, 13);

		if (TC.Equal(3, Found->Array.Num()))
		{
			TC.Equal(13, Found->Array[0].Data.Get<FTestTableRow1>()->Value);
			TC.Equal(13, Found->Array[1].Data.Get<FTestTableRow1>()->Value);
			TC.Equal(13, Found->Array[2].Data.Get<FTestTableRow1>()->Value);
		}

		if (TC.Equal(3, Found->Map.Num()))
		{
			TC.Equal(13, Found->Map[1].Data.Get<FTestTableRow1>()->Value);
			TC.Equal(13, Found->Map[2].Data.Get<FTestTableRow1>()->Value);
			TC.Equal(13, Found->Map[3].Data.Get<FTestTableRow1>()->Value);
		}
	});

	// Ensures that FVulDataPtr conversions and shared ptr generation operate correctly.
	VulTest::Case(this, "Pointer behaviour", [](VulTest::TC TC)
	{
		auto DT1 = NewObject<UDataTable>();
		DT1->RowStruct = FTestTableRow1::StaticStruct();
		auto DT2 = NewObject<UDataTable>();
		DT2->RowStruct = FTestTableRow2::StaticStruct();

		DT1->AddRow(FName("Table1Row1"), FTestTableRow1(10));

		auto TTR2 = FTestTableRow2();
		TTR2.ARef = FVulDataPtr("Table1Row1");
		DT2->AddRow(FName("Table2Row1"), TTR2);

		auto Repo = NewObject<UVulDataRepository>();

		Repo->DataTables = {
			{FName("T1"), DT1},
			{FName("T2"), DT2},
		};

		const auto Row2 = Repo->FindChecked<FTestTableRow2>("T2", "Table2Row1");

		TC.Equal(
			FString::Printf(TEXT("%p"), Row2.SharedPtr().Get()),
			FString::Printf(TEXT("%p"), Row2.SharedPtr().Get()),
			"shared ptr is created once"
		);

		TC.NotEqual(
			FString::Printf(TEXT("%p"), Row2.Get()),
			FString::Printf(TEXT("%p"), Row2.SharedPtr().Get()),
			"shared ptr is not raw ptr"
		);

		const auto ReferencedRow = Cast<FTestTableRow1>(Row2->ARef);
		// Check again with the referenced table.
		TC.Equal(
			FString::Printf(TEXT("%p"), ReferencedRow.SharedPtr().Get()),
			FString::Printf(TEXT("%p"), ReferencedRow.SharedPtr().Get()),
			"referenced shared ptr is created once"
		);

		TC.NotEqual(
			FString::Printf(TEXT("%p"), ReferencedRow.SharedPtr().Get()),
			FString::Printf(TEXT("%p"), ReferencedRow.Get()),
			"referenced shared ptr is not raw ptr"
		);
	});

	return !HasAnyErrors();
}
