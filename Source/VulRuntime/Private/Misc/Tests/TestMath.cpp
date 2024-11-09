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

		Ddt.Run("#1", {
			.Plane = FPlane(FVector(0, -1, 0), -1),
			.LineStart = FVector(1, 0, 0),
			.Direction = FVector(1, -1, 0).Rotation(),
			.ExpectedResult = {FVector(2, -1, 0)},
		});

		Ddt.Run("#2", {
			.Plane = FPlane(FVector(0, -1, 0), -3),
			.LineStart = FVector(1, 0, 0),
			.Direction = FVector(1, -1, 0).Rotation(),
			.ExpectedResult = {FVector(4, -3, 0)},
		});

		Ddt.Run("#3", {
			.Plane = FPlane(FVector(1, 1, 0).GetSafeNormal(), 0),
			.LineStart = FVector(-2, -1, 0),
			.Direction = FVector(4, -1, 0).Rotation(),
			.ExpectedResult = {FVector(2, -2, 0)},
		});

		Ddt.Run("no intersection", {
			.Plane = FPlane(FVector(0, -1, 0), -1),
			.LineStart = FVector(1, 0, 0),
			.Direction = FVector(1, 0, 0).Rotation(),
			.ExpectedResult = {},
		});
	}

	{
		struct Data
		{
			FVector A;
			FVector B;
			FVector P;
			FVector ExpectedResult;
		};

		auto Ddt = DDT<Data>(this, "ClosestPointOnLineSegment", [](TC Test, Data Case)
		{
			const auto Result = FVulMath::ClosestPointOnLineSegment(Case.A, Case.B, Case.P);

			Test.Equal(Result, Case.ExpectedResult);
		});

		Ddt.Run("#1", {
			.A = FVector(0, 0, 0),
			.B = FVector(1, 0, 0),
			.P = FVector(2, 0, 0),
			.ExpectedResult = FVector(1, 0, 0)}
		);

		Ddt.Run("#2", {
			.A = FVector(0, 0, 0),
			.B = FVector(2, 0, 0),
			.P = FVector(1, 1, 0),
			.ExpectedResult = FVector(1, 0, 0)}
		);

		Ddt.Run("#3", {
			.A = FVector(0, 0, 0),
			.B = FVector(2, 0, 0),
			.P = FVector(0.75, 1, 0),
			.ExpectedResult = FVector(0.75, 0, 0)}
		);
	}

	{
		struct Data
		{
			FVector A;
			FVector B;
			float T;
			float Distance;
			FVector Plane;
			TArray<FVector> ExpectedResult;
		};

		auto Ddt = DDT<Data>(this, "EitherSideOfLine", [](TC Test, Data Case)
		{
			const auto Result = FVulMath::EitherSideOfLine(Case.A, Case.B, Case.T, Case.Plane, Case.Distance);

			Test.Equal(Result, Case.ExpectedResult);

			// Quick sanity checks.
			Test.Equal((Result[0] - Result[1]).Size(), static_cast<double>(Case.Distance * 2), "distance check");
		});

		Ddt.Run("xy-plane-diagonal-at-1", {
			.A=FVector(0, 0, 0),
			.B=FVector(1, 0, 0),
			.T=1.f,
			.Distance=1,
			.Plane=FVector(0, 0, 1),
			.ExpectedResult={FVector(1, 1, 0), FVector(1, -1, 0)},
		});

		Ddt.Run("xy-plane-diagonal-at-.5", {
			.A=FVector(0, 0, 0),
			.B=FVector(2, 2, 0),
			.T=.5f,
			.Distance=1,
			.Plane=FVector(0, 0, 1),
			.ExpectedResult={FVector(0.2929, 1.7071, 0), FVector(1.7071, 0.2929, 0)},
		});

		Ddt.Run("xz-plane-diagonal-at-0", {
			.A=FVector(0, 0, 0),
			.B=FVector(2, 0, 2),
			.T=0.f,
			.Distance=1,
			.Plane=FVector(0, 1, 0),
			.ExpectedResult={FVector(0.7071, 0, -0.7071), FVector(-0.7071, 0, 0.7071)},
		});

		Ddt.Run("xz-plane-diagonal-at-.5", {
			.A=FVector(0, 0, 0),
			.B=FVector(2, 2, 0),
			.T=.5f,
			.Distance=1,
			.Plane=FVector(0, 1, 0),
			.ExpectedResult={FVector(1, 1, -1), FVector(1, 1, 1)},
		});

		Ddt.Run("from-tile-test", {
			.A=FVector(-8.66025, 15, 0),
			.B=FVector(18.32050, 0, 0),
			.T=.5f,
			.Distance=.1,
			.Plane=FVector(0, 0, 1),
			.ExpectedResult={FVector(4.8787, 7.5874, 0), FVector(4.7815, 7.4125, 0)},
		});
	}

	{
		struct Data
		{
			FBox Box;
			FVector Position;
			FVector Expected;
		};

		auto Ddt = DDT<Data>(this, "PointInBox", [](TC Test, Data Case)
		{
			Test.Equal(FVulMath::PointInBox(Case.Box, Case.Position), Case.Expected);
		});

		const auto Box = FBox(FVector(0, 0, 0), FVector(2, 2, 2));

		Ddt.Run("0,0,0", {.Box = Box, .Position = {0, 0, 0}, .Expected = {0, 0, 0}});
		Ddt.Run("0.5,0,0", {.Box = Box, .Position = {0.5, 0, 0}, .Expected = {1, 0, 0}});
		Ddt.Run("0.5,0.5,0.5", {.Box = Box, .Position = {0.5, 0.5, 0.5}, .Expected = {1, 1, 1}});
		Ddt.Run("1,0.75,1", {.Box = Box, .Position = {1, .75, 1}, .Expected = {2, 1.5, 2}});
		Ddt.Run("1,1,1", {.Box = Box, .Position = {1, 1, 1}, .Expected = {2, 2, 2}});
		Ddt.Run("-1,1,2", {.Box = Box, .Position = {-1, 1, 2}, .Expected = {-2, 2, 4}});
	}

	return !HasAnyErrors();
}
