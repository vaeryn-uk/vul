#include "TestCase.h"
#include "Misc/AutomationTest.h"
#include "Misc/VulVectorPath.h"

using namespace VulTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestVectorPath,
	"VulRuntime.Misc.TestVectorPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestVectorPath::RunTest(const FString& Parameters)
{
	{
		struct Data
		{
			TArray<FVector> Path;
			float Alpha;
			FVector Expected;
		};
		auto Ddt = DDT<Data>(this, "Interpolate path", [](TC Test, Data Case)
		{
			const auto Path = FVulVectorPath(Case.Path);
			Test.NearlyEqual(Path.Interpolate(Case.Alpha), Case.Expected);
		});

		TArray<FVector> XPath = {{0, 0, 0}, {1, 0, 0}};
		Ddt.Run("x=1, alpha=0", {.Path = XPath, .Alpha = 0, .Expected = {0, 0, 0}});
		Ddt.Run("x=1, alpha=0.5", {.Path = XPath, .Alpha = 0.5, .Expected = {0.5, 0, 0}});
		Ddt.Run("x=1, alpha=1", {.Path = XPath, .Alpha = 1, .Expected = {1, 0, 0}});
		Ddt.Run("x=1, alpha=1.5", {.Path = XPath, .Alpha = 1.5, .Expected = {1, 0, 0}});
		Ddt.Run("x=1, alpha=-10", {.Path = XPath, .Alpha = -10, .Expected = {0, 0, 0}});

		TArray<FVector> XYPath = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}};
		Ddt.Run("x=1,y=1, alpha=0", {.Path = XYPath, .Alpha = 0, .Expected = {0, 0, 0}});
		Ddt.Run("x=1,y=1, alpha=0.25", {.Path = XYPath, .Alpha = 0.25, .Expected = {0.5, 0, 0}});
		Ddt.Run("x=1,y=1, alpha=0.5", {.Path = XYPath, .Alpha = 0.5, .Expected = {1, 0, 0}});
		Ddt.Run("x=1,y=1, alpha=0.75", {.Path = XYPath, .Alpha = 0.75, .Expected = {1, 0.5, 0}});
		Ddt.Run("x=1,y=1, alpha=1", {.Path = XYPath, .Alpha = 1, .Expected = {1, 1, 0}});

		TArray<FVector> InvalidPath = {{1, 1, 1}};
		Ddt.Run("invalid alpha=0", {.Path = InvalidPath, .Alpha = 0, .Expected = {0, 0, 0}});
		Ddt.Run("invalid alpha=0.5", {.Path = InvalidPath, .Alpha = 0.5, .Expected = {0, 0, 0}});
		Ddt.Run("invalid alpha=1", {.Path = InvalidPath, .Alpha = 1, .Expected = {0, 0, 0}});
	}

	return !HasAnyErrors();
}
