#include "GeomTools.h"
#include "TestCase.h"
#include "Misc/AutomationTest.h"
#include "Misc/VulMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestMath,
	"VulRuntime.Misc.TestMath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

using namespace VulTest;

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

	{
		struct Data
		{
			FPlane Plane;
			FVector LineStart;
			FRotator Direction;
			TOptional<FVector> ExpectedResult;
		};

		auto Ddt = DDT<Data>(this, "LinePlaneIntersection", [](TC Test, Data Case)
		{
			const auto Result = FVulMath::LinePlaneIntersection(Case.LineStart, Case.Direction, Case.Plane);

			Test.Equal(Result, Case.ExpectedResult);
		});

		Ddt.Run("simple", {
			.Plane = FPlane(FVector(0, -1, 0), -1),
			.LineStart = FVector(1, 0, 0),
			.Direction = FVector(1, -1, 0).Rotation(),
			.ExpectedResult = {FVector(2, -1, 0)},
		});

		Ddt.Run("no intersection", {
			.Plane = FPlane(FVector(0, -1, 0), -1),
			.LineStart = FVector(1, 0, 0),
			.Direction = FVector(1, 0, 0).Rotation(),
			.ExpectedResult = {},
		});
	}

	return !HasAnyErrors();
}
