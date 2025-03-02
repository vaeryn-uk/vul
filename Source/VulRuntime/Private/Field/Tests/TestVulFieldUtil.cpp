#include "TestCase.h"
#include "Field/VulFieldUtil.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestVulFieldUtil,
	"VulRuntime.Field.TestVulFieldUtil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestVulFieldUtil::RunTest(const FString& Parameters)
{
	{
		struct Data
		{
			VulRuntime::Field::FPath Path;
			FString Match;
			bool ExpectedMatch;
		};
		
		auto Ddt = VulTest::DDT<Data>(this, "path match", [](const VulTest::TestCase& TC, const Data& Data)
		{
			VTC_MUST_EQUAL(
				VulRuntime::Field::PathMatch(Data.Path, Data.Match),
				Data.ExpectedMatch,
				FString::Printf(TEXT("%s matches %s"), *Data.Match, *VulRuntime::Field::PathStr(Data.Path))
			);
		});

		Ddt.Run("root", Data{
			.Path = {},
			.Match = ".",
			.ExpectedMatch = true,
		});

		Ddt.Run("single-property", Data{
			.Path = {VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "foo")},
			.Match = ".foo",
			.ExpectedMatch = true,
		});

		Ddt.Run("multi-property", Data{
			.Path = {
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "foo"),
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "bar"),
			},
			.Match = ".foo.bar",
			.ExpectedMatch = true,
		});

		Ddt.Run("wildcard-property", Data{
			.Path = {
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "foo"),
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "bar"),
			},
			.Match = ".foo.*",
			.ExpectedMatch = true,
		});

		Ddt.Run("wildcard-array", Data{
			.Path = {
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "foo"),
				VulRuntime::Field::FPathItem(TInPlaceType<int>(), 13),
			},
			.Match = ".foo[*]",
			.ExpectedMatch = true,
		});

		Ddt.Run("wildcard-array-and-prop", Data{
			.Path = {
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "foo"),
				VulRuntime::Field::FPathItem(TInPlaceType<int>(), 13),
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "bar"),
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "qux"),
			},
			.Match = ".foo[*].bar.*",
			.ExpectedMatch = true,
		});

		Ddt.Run("no-match-1", Data{
			.Path = {
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "foo"),
				VulRuntime::Field::FPathItem(TInPlaceType<int>(), 13),
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "bar"),
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "qux"),
			},
			.Match = ".foo[*].bar.baz",
			.ExpectedMatch = false,
		});

		Ddt.Run("no-match-2", Data{
			.Path = {
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "foo"),
				VulRuntime::Field::FPathItem(TInPlaceType<int>(), 13),
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "bar"),
				VulRuntime::Field::FPathItem(TInPlaceType<FString>(), "qux"),
			},
			.Match = ".foo[9].bar.qux",
			.ExpectedMatch = false,
		});
	}

	return true;
}
