#include "Misc/AutomationTest.h"
#include "VulEditorDataTableTestStructs.h"
#include "DataTable/VulDataTableSource.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestDataTableSource,
	"VulEditor.DataTable.TestDataTableSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestDataTableSource::RunTest(const FString& Parameters)
{
	auto Table = NewObject<UDataTable>();
	Table->RowStruct = FTestStruct::StaticStruct();

	auto Source = NewObject<UVulDataTableSource>();
	FDirectoryPath Dir;
	Dir.Path = FPaths::Combine(FPaths::ProjectPluginsDir(), "Vul/Source/VulEditor/Private/DataTable/Tests");
	Source->DataTable = Table;
	Source->Directory = Dir;
	Source->FilePatterns = {"test_data.yaml"};

	Source->Import();

	TArray<FTestStruct*> Rows;
	Table->GetAllRows(TEXT("Source test"), Rows);
	TestEqual("Source import: row count", Rows.Num(), 2);

	TestEqual("Source import row 1: num", Rows[0]->Num, 13);
	TestEqual("Source import row 1: rowname", Rows[0]->RowName.ToString(), TEXT("row1"));
	TestEqual("Source import row 2: num", Rows[1]->Num, -1);
	TestEqual("Source import row 2: rowname", Rows[1]->RowName.ToString(), TEXT("row2"));

	// FTestStruct
	// Make the test pass by returning true, or fail by returning false.
	return true;
}
