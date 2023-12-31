#include "TestCase.h"
#include "Hexgrid/Addr.h"
#include "Hexgrid/Util.h"
#include "Misc/AutomationTest.h"

using namespace VulTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestUtil,
	"Vul.VulRuntime.Private.Hexgrid.Tests.TestUtil",
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

	return true;
}
