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
		TTR2.ARef.RowName = FName("Table1Row1");
		DT2->AddRow(FName("Table2Row1"), TTR2);

		auto Repo = NewObject<UVulDataRepository>();

		Repo->DataTables = {
			{FName("T1"), DT1},
			{FName("T2"), DT2},
		};

		{
			auto Row = Repo->FindChecked<FTestTableRow1>(FName("T1"), FName("Table1Row1"));
			TestEqual("Repository FindChecked", Row->Value, 10);
		}

		{
			auto Row = Repo->FindChecked<FTestTableRow2>(FName("T2"), FName("Table2Row1"));
			TestEqual("Repository FindChecked", Row->ARef.FindChecked<FTestTableRow1>()->Value, 10);
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
		Parent.CircularProperty.RowName = TEXT("ChildRow1");

		Parent.CircularArray.Add(FVulDataRef("ChildRow1"));
		Parent.CircularArray.Add(FVulDataRef("ChildRow2"));

		Parent.CircularMap.Add("a", FCircDepIncluder(15, FVulDataRef("ChildRow2")));
		Parent.CircularMap.Add("b", FCircDepIncluder(25, FVulDataRef("ChildRow3")));

		DT->AddRow(FName("ParentRow"), Parent);

		auto Repo = NewObject<UVulDataRepository>();

		Repo->DataTables = {
			{FName("CircTable"), DT},
		};

		auto FoundParent = Repo->FindChecked<FCircDep>(FName("CircTable"), FName("ParentRow"));
		TestEqual("CircDep parent", FoundParent->Value, 10);
		TestEqual("CircDep property", FoundParent->CircularProperty.FindChecked<FCircDep>()->Value, 20);
		if (TestEqual("CircDep array length", FoundParent->CircularArray.Num(), 2))
		{
			TestEqual("CircDep array[0]", FoundParent->CircularArray[0].FindChecked<FCircDep>()->Value, 20);
			TestEqual("CircDep array[1]", FoundParent->CircularArray[1].FindChecked<FCircDep>()->Value, 30);
		}

		if (TestEqual("CircDep map length", FoundParent->CircularMap.Num(), 2))
		{
			if (TestTrue("CircDep map[\"a\"]", FoundParent->CircularMap.Contains("a")))
			{
				TestEqual("CircDep map[\"a\"] value", FoundParent->CircularMap["a"].SomeValue, 15);
				TestEqual("CircDep map[\"a\"] ref", FoundParent->CircularMap["a"].CircularProperty.FindChecked<FCircDep>()->Value, 30);
			}

			if (TestTrue("CircDep map[\"b\"]", FoundParent->CircularMap.Contains("b")))
			{
				TestEqual("CircDep map[\"b\"] value", FoundParent->CircularMap["b"].SomeValue, 25);
				TestEqual("CircDep map[\"b\"] ref", FoundParent->CircularMap["b"].CircularProperty.FindChecked<FCircDep>()->Value, 40);
			}
		}
	}

	return !HasAnyErrors();
}
