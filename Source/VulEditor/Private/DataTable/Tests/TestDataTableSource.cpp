#include "Misc/AutomationTest.h"
#include "VulEditorDataTableTestStructs.h"
#include "DataTable/VulDataTableSource.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestDataTableSource,
	"VulEditor.DataTable.TestDataTableSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

void TestAutoPopulateRowName(TestDataTableSource* TestCase);
void TestMultiStructImport(TestDataTableSource* TestCase);
void TestDataRefParsing(TestDataTableSource* TestCase);
UVulDataTableSource* CreateSource(const TCHAR* FilePattern, UDataTable* DataTable);

bool TestDataTableSource::RunTest(const FString& Parameters)
{
	TestAutoPopulateRowName(this);
	TestMultiStructImport(this);
	TestDataRefParsing(this);

	return !HasAnyErrors();
}

void TestAutoPopulateRowName(TestDataTableSource* TestCase)
{
	auto Table = NewObject<UDataTable>();
	Table->RowStruct = FTestStruct::StaticStruct();

	auto Source = CreateSource(TEXT("test_data.yaml"), Table);

	Source->Import();

	TArray<FTestStruct*> Rows;
	Table->GetAllRows(TEXT("Auto RowName test"), Rows);
	TestCase->TestEqual("Auto RowName: row count", Rows.Num(), 2);

	TestCase->TestEqual("Auto RowName row 1: num", Rows[0]->Num, 13);
	TestCase->TestEqual("Auto RowName row 1: rowname", Rows[0]->RowName.ToString(), TEXT("row1"));
	TestCase->TestEqual("Auto RowName row 2: num", Rows[1]->Num, -1);
	TestCase->TestEqual("Auto RowName row 2: rowname", Rows[1]->RowName.ToString(), TEXT("row2"));
}

void TestMultiStructImport(TestDataTableSource* TestCase)
{
	auto CharactersTable = NewObject<UDataTable>();
	CharactersTable->RowStruct = FTestCharacter::StaticStruct();

	auto WeaponsTable = NewObject<UDataTable>();
	WeaponsTable->RowStruct = FTestWeapon::StaticStruct();

	auto CharacterSource = CreateSource(TEXT("multi_struct_data.yaml"), CharactersTable);
	CharacterSource->TopLevelKey = "character";

	CharacterSource->Import();

	TArray<FTestCharacter*> CharacterRows;
	CharactersTable->GetAllRows(TEXT("MultiStruct test"), CharacterRows);
	if (TestCase->TestEqual("MultiStruct Characters: row count", CharacterRows.Num(), 1))
	{
		TestCase->TestEqual("MultiStruct Characters: row #1 name", CharacterRows[0]->Name.ToString(), "john");
		TestCase->TestEqual("MultiStruct Characters: row #1 hp", CharacterRows[0]->Hp, 50);
		TestCase->TestEqual("MultiStruct Characters: row #1 strength", CharacterRows[0]->Strength, 5);
	}

	auto WeaponsSource = CreateSource(TEXT("multi_struct_data.yaml"), WeaponsTable);
	WeaponsSource->TopLevelKey = "weapon";

	WeaponsSource->Import();

	TArray<FTestWeapon*> WeaponRows;
	WeaponsTable->GetAllRows(TEXT("MultiStruct test"), WeaponRows);
	if (TestCase->TestEqual("MultiStruct Weapons: row count", WeaponRows.Num(), 2))
	{
		TestCase->TestEqual("MultiStruct Weapons: row #1 damage", WeaponRows[0]->Damage, 5);
		TestCase->TestEqual("MultiStruct Weapons: row #1 minStrength", WeaponRows[0]->MinStrength, 3);
		TestCase->TestEqual("MultiStruct Weapons: row #2 damage", WeaponRows[1]->Damage, 3);
		TestCase->TestEqual("MultiStruct Weapons: row #2 minStrength", WeaponRows[1]->MinStrength, 1);
	}
}

void TestDataRefParsing(TestDataTableSource* TestCase)
{
	auto Table = NewObject<UDataTable>();
	Table->RowStruct = FTestDataRef::StaticStruct();

	auto Source = CreateSource(TEXT("data_ref_parsing.yaml"), Table);

	Source->Import();

	TArray<FTestDataRef*> Rows;
	Table->GetAllRows(TEXT("MultiStruct test"), Rows);
	if (TestCase->TestEqual("Data ref row count", Rows.Num(), 1))
	{
		TestCase->TestEqual("Data Ref parsed", Rows[0]->Ref.RowName.ToString(), "somevalue");
	}
}

UVulDataTableSource* CreateSource(const TCHAR* FilePattern, UDataTable* DataTable)
{
	auto Source = NewObject<UVulDataTableSource>();

	FDirectoryPath Dir;
	Dir.Path = FPaths::Combine(FPaths::ProjectPluginsDir(), "Vul/Source/VulEditor/Private/DataTable/Tests");

	Source->DataTable = DataTable;
	Source->Directory = Dir;
	Source->FilePatterns = {FilePattern};

	return Source;
}
