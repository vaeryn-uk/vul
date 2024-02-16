#include "TestCase.h"
#include "Hexgrid/VulHexAddr.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestAddr,
	"VulRuntime.Hexgrid.TestAddr",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestAddr::RunTest(const FString& Parameters)
{
	{ // Test Rotation towards.
		struct Data { FVulHexAddr From; FVulHexAddr To; FVulHexRotation Expected;};

		auto Ddt = VulTest::DDT<Data>(this, "RotationTowards", [](const VulTest::TestCase& TC, const Data& Data)
		{
			TC.Equal(Data.From.RotationTowards(Data.To).GetValue(), Data.Expected.GetValue());
		});

		Ddt.Run("0,0 -> 1,-1", {.From={0, 0}, .To = {1, -1}, .Expected=0});
		Ddt.Run("0,0 -> 1,0", {.From={0, 0}, .To = {1, 0}, .Expected=1});
		Ddt.Run("0,0 -> 0,1", {.From={0, 0}, .To = {0, 1}, .Expected=2});
		Ddt.Run("0,0 -> -1,1", {.From={0, 0}, .To = {-1, 1}, .Expected=3});
		Ddt.Run("0,0 -> -1,0", {.From={0, 0}, .To = {-1, 0}, .Expected=4});
		Ddt.Run("0,0 -> 0,-1", {.From={0, 0}, .To = {0, -1}, .Expected=5});
	}

	return !HasAnyErrors();
}
