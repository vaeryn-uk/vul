#include "TestCase.h"
#include "Hexgrid/VulHexAddr.h"
#include "Hexgrid/VulHexUtil.h"
#include "Misc/AutomationTest.h"

using namespace VulTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestUtil,
	"VulRuntime.Hexgrid.TestUtil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestUtil::RunTest(const FString& Parameters)
{
	{
		struct Data{ int HexSize; FVulHexAddr Addr; FVector Expected; };

		auto Ddt = DDT<Data>(this, TEXT("Project"), [](TC Test, Data Case) {
			Test.Equal(VulRuntime::Hexgrid::Project(Case.Addr, Case.HexSize), Case.Expected);
		});

		Ddt.Run(TEXT("0,0"), {6, FVulHexAddr(0, 0), FVector(0, 0, 0)});
		Ddt.Run(TEXT("1,0"), {6, FVulHexAddr(1, 0), FVector(10.3923, 0, 0)});
		Ddt.Run(TEXT("1,-1"), {6, FVulHexAddr(1, -1), FVector(5.1961, 9, 0)});
		Ddt.Run(TEXT("1,-2"), {6, FVulHexAddr(1, -2), FVector(0, 18, 0)});
		Ddt.Run(TEXT("-3,3"), {6, FVulHexAddr(-3, 3), FVector(-15.58845, -27, 0)});
	}

	{
		struct Data{ int HexSize; FBox MeshBox; FTransform Expected; };

		auto Ddt = DDT<Data>(this, TEXT("MeshTransform"), [](TC Test, Data Case)
		{
			Test.Equal(
				VulRuntime::Hexgrid::CalculateMeshTransformation(Case.MeshBox, Case.HexSize),
				Case.Expected
			);
		});

		Ddt.Run(TEXT("Simple"), {
			6,
			FBox(FVector(0, 0, 0), FVector(.8660, 1, .1)),
			FTransform(
				FQuat::Identity,
				FVector(0, 0, 0),
				FVector(12, 12, 1)
			)
		});

		Ddt.Run(TEXT("Larger mesh - scale down"), {
			25,
			FBox(FVector(0, 0, 0), FVector(86.64, 100, .1)),
			FTransform(
				FQuat::Identity,
				FVector(0, 0, 0),
				FVector(.5, .5, 1)
			)
		});
	}

	{
		struct Data{ int HexSize; FVector WorldPos; FVulHexAddr Expected; };

		auto Ddt = DDT<Data>(this, TEXT("Deproject"), [](TC Test, Data Case)
		{
			Test.Equal(
				VulRuntime::Hexgrid::Deproject(Case.WorldPos, Case.HexSize).ToString(),
				Case.Expected.ToString()
			);
		});

		Ddt.Run(TEXT("Origin"), {6, FVector(0, 0, 0), FVulHexAddr(0, 0)});
		Ddt.Run(TEXT("2, 2"), {6, FVector(2, 2, 0), FVulHexAddr(0, 0)});
		Ddt.Run(TEXT("-2, -2"), {6, FVector(-2, -2, 0), FVulHexAddr(0, 0)});
		Ddt.Run(TEXT("2, 8"), {6, FVector(2, 8, 0), FVulHexAddr(1, -1)});
	}

	{
		// Triangles.
		struct Data{ int HexSize; FVulHexAddr Addr; TArray<TArray<FVector>> Expected; };

		auto Ddt = DDT<Data>(this, TEXT("Triangles"), [](TC Test, Data Case)
		{
			const auto Result = VulRuntime::Hexgrid::Triangles(Case.Addr, Case.HexSize);
			if (Test.Equal(Result.Num(), 6))
			{
				for (auto N = 0; N < 6; ++N)
				{
					Test.Equal(Result[N][0], Case.Expected[N][0]);
					Test.Equal(Result[N][1], Case.Expected[N][1]);
					Test.Equal(Result[N][2], Case.Expected[N][2]);
				}
			}
		});

		Ddt.Run(TEXT("Origin"), {5, FVulHexAddr(0, 0), {
			{{4.3301, -2.5, 0}, {0, 0, 0}, {4.3301, 2.5, 0}},
			{{4.3301, 2.5, 0}, {0, 0, 0}, {0, 5, 0}},
			{{0, 5, 0}, {0, 0, 0}, {-4.3301, 2.5, 0}},
			{{-4.3301, 2.5, 0}, {0, 0, 0}, {-4.3301, -2.5, 0}},
			{{-4.3301, -2.5, 0}, {0, 0, 0}, {0, -5, 0}},
			{{0, -5, 0}, {0, 0, 0}, {4.3301, -2.5, 0}},
		}});
	}

	{ // Points.
		struct Data{ int HexSize; FVulHexAddr Addr; TArray<FVector> Expected; float Scale = 1; };

		auto Ddt = DDT<Data>(this, "Points", [](TC Test, Data Case)
		{
			const auto Result = VulRuntime::Hexgrid::Points(Case.Addr, Case.HexSize, Case.Scale);
			if (Test.Equal(Result.Num(), 6))
			{
				for (auto N = 0; N < 6; ++N)
				{
					Test.Equal(Result[N], Case.Expected[N], FString::FromInt(N));
				}
			}
		});

		Ddt.Run(TEXT("Origin"), {5, FVulHexAddr(0, 0), {
			FVector(4.3301, 2.5, 0),
			FVector(0, 5, 0),
			FVector(-4.3301, 2.5, 0),
			FVector(-4.3301, -2.5, 0),
			FVector(0, -5, 0),
			FVector(4.3301, -2.5, 0),
		}});

		Ddt.Run(TEXT("Origin, larger"), {5 * 2, FVulHexAddr(0, 0), {
			FVector(4.3301 * 2, 2.5 * 2, 0),
			FVector(0, 5 * 2, 0),
			FVector(-4.3301 * 2, 2.5 * 2, 0),
			FVector(-4.3301 * 2, -2.5 * 2, 0),
			FVector(0, -5 * 2, 0),
			FVector(4.3301 * 2, -2.5 * 2, 0),
		}});

		Ddt.Run(TEXT("1,-1"), {5, FVulHexAddr(1, -1), {
			FVector(4.3301 * 2, 10, 0),
			FVector(4.3301, 12.5, 0),
			FVector(0, 10, 0),
			FVector(0, 5, 0),
			FVector(4.3301, 2.5, 0),
			FVector(4.3301 * 2, 5, 0),
		}});

		Ddt.Run(TEXT("1,-1 scaled down"), {5, FVulHexAddr(1, -1), {
			FVector(6.4951, 8.75, 0),
			FVector(4.3301, 10, 0),
			FVector(2.165, 8.75, 0),
			FVector(2.165, 6.250, 0),
			FVector(4.3301, 5.000, 0),
			FVector(6.4951, 6.250, 0),
		}, .5});
	}

	return true;
}
