#include "GeomTools.h"
#include "Misc/AutomationTest.h"
#include "Misc/VulMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestMath,
	"VulRuntime.Misc.TestMath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestMath::RunTest(const FString& Parameters)
{
	{
		const TArray<FVector> Triangle = {
			{0, 0, 0},
			{1, 1, 1},
			{2, 2, 2},
		};

		auto Failures = 0;

		for (auto N = 0; N < 999; N++)
		{
			bool Ok = FGeomTools::PointInTriangle(
				FVector3f(Triangle[0]),
				FVector3f(Triangle[0]),
				FVector3f(Triangle[0]),
				FVector3f(FVulMath::RandomPointInTriangle(Triangle))
			);

			if (!Ok)
			{
				Failures++;
			}
		}

		if (Failures > 0)
		{
			AddError(FString::Printf(TEXT("PointInTriangle failed on %d randomized tests"), Failures));
		}
	}

	return !HasAnyErrors();
}
