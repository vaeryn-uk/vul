#include "TestCase.h"
#include "Hexgrid/VulHexAddr.h"
#include "Hexgrid/VulHexShape.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestShape,
	"VulRuntime.Hexgrid.TestShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestShape::RunTest(const FString& Parameters)
{
	{
		struct Data
		{
			TArray<FVulHexRotation> InDirections;
			FVulHexAddr Origin;
			FVulHexRotation Rotation;
			TArray<FVulHexAddr> ExpectedTiles;
		};

		auto Ddt = VulTest::DDT<Data>(this, "Projection", [](const VulTest::TestCase& TC, const Data& Data)
		{
			const auto Shape = FVulHexVectorShape(Data.InDirections);

			TC.Equal(Shape.Project(Data.Origin, Data.Rotation), Data.ExpectedTiles);
		});

		Ddt.Run("Empty shape", {
			{},
			FVulHexAddr(),
			FVulHexRotation(),
			{FVulHexAddr()}
		});

		Ddt.Run("Straight line 2, origin, 0 rotation", {
			{0, 0},
			FVulHexAddr(),
			FVulHexRotation(),
			{FVulHexAddr(), FVulHexAddr(1, 0), FVulHexAddr(2, 0)}
		});

		Ddt.Run("Straight line 2, origin, 3 rotation", {
			{0, 0},
			FVulHexAddr(),
			3,
			{FVulHexAddr(), FVulHexAddr(-1, 0), FVulHexAddr(-2, 0)}
		});

		Ddt.Run("Perp line 2, origin, 0 rotation", {
			{0, 0},
			FVulHexAddr(),
			3,
			{FVulHexAddr(), FVulHexAddr(-1, 0), FVulHexAddr(-2, 0)}
		});
	}

	return !HasAnyErrors();
}
