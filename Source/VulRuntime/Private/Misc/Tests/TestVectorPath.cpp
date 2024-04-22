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

	{
		struct Data
		{
			TArray<FVector> Path;
			float TurnDegsPerWorldUnit;
			float Samples;
			TArray<FVector> Expected;
		};
		auto Ddt = DDT<Data>(this, "Curve", [](TC Test, Data Case)
		{
			const auto Path = FVulVectorPath(Case.Path);
			const auto Curved = Path.Curve(Case.TurnDegsPerWorldUnit, Case.Samples);

			Test.Equal(Curved.GetPoints(), Case.Expected);
		});

		Ddt.Run("#1", {
			.Path = {FVector(0, 0, 0), FVector(4, 0, 0), FVector(4, 4, 0)},
			.TurnDegsPerWorldUnit = 45,
			.Samples = 4,
			.Expected = {
				FVector(0, 0, 0),
				FVector(4, 0, 0),
				FVector(4.7071, 0.7071, 0),
				FVector(4.7071, 1.7071, 0),
				FVector(4, 4, 0),
			},
		});
	}

	{
		struct Data
		{
			TArray<FVector> Path;
			TArray<FVector> Expected;
		};
		auto Ddt = DDT<Data>(this, "Simplify", [](TC Test, Data Case)
		{
			const auto Path = FVulVectorPath(Case.Path);

			Test.Equal(Path.Simplify().GetPoints(), Case.Expected);
		});

		Ddt.Run("#1", {
			.Path = {FVector(0, 0, 0), FVector(1, 0, 0), FVector(2, 0, 0)},
			.Expected = {FVector(0, 0, 0), FVector(2, 0, 0)}
		});

		Ddt.Run("#2", {
			.Path = {FVector(0, 0, 0), FVector(.5, 0, 0), FVector(1, 0, 0), FVector(1.5, 0, 0), FVector(2, 0, 0)},
			.Expected = {FVector(0, 0, 0), FVector(2, 0, 0)}
		});

		Ddt.Run("#3", {
			.Path = {FVector(0, 0, 0), FVector(1, 0, 0), FVector(2, 0, 0), FVector(2, 1, 0), FVector(2, 2, 0)},
			.Expected = {FVector(0, 0, 0), FVector(2, 0, 0), FVector(2, 2, 0)}
		});

		Ddt.Run("#4", {
			.Path = {FVector(0, 0, 0), FVector(1, 0, 0), FVector(1, 1, 0), FVector(1, 2, 0), FVector(2, 2, 0)},
			.Expected = {FVector(0, 0, 0), FVector(1, 0, 0), FVector(1, 2, 0), FVector(2, 2, 0)}
		});
	}

	return !HasAnyErrors();
}
