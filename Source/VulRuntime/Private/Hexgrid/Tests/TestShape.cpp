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
	{ // Test Rotate().
		struct Data
		{
			TArray<FVulHexAddr> Tiles;
			FVulHexRotation Rotation;
			FString ExpectedTiles;
		};

		auto Ddt = VulTest::DDT<Data>(this, "Rotate", [](const VulTest::TestCase& TC, const Data& Data)
		{
			const auto Shape = FVulHexShape(Data.Tiles);

			TC.Equal(Shape.Rotate(Data.Rotation).ToString(), Data.ExpectedTiles);
		});

		Ddt.Run("Empty shape", {.Tiles={}, .ExpectedTiles={}});

		const TArray<FVulHexAddr> Straight2 = {{1, -1}, {2, -2}};
		Ddt.Run("Straight2, origin, 0 rotation", {.Tiles=Straight2, .ExpectedTiles="(1 -1 0), (2 -2 0)"});
		Ddt.Run("Straight2, origin, 1 rotation", {.Tiles=Straight2, .Rotation=1, .ExpectedTiles="(1 0 -1), (2 0 -2)",});
		Ddt.Run("Straight2, origin, 2 rotation", {.Tiles=Straight2, .Rotation=2, .ExpectedTiles="(0 1 -1), (0 2 -2)",});
		Ddt.Run("Straight2, origin, 3 rotation", {.Tiles=Straight2, .Rotation=3, .ExpectedTiles="(-1 1 0), (-2 2 0)",});
		Ddt.Run("Straight2, origin, 4 rotation", {.Tiles=Straight2, .Rotation=4, .ExpectedTiles="(-1 0 1), (-2 0 2)",});
		Ddt.Run("Straight2, origin, 5 rotation", {.Tiles=Straight2, .Rotation=5, .ExpectedTiles="(0 -1 1), (0 -2 2)",});

		const TArray<FVulHexAddr> Perp2 = {{1, -1}, {1, 0}};
		Ddt.Run("Perp2, origin, 0 rotation", {.Tiles=Perp2, .ExpectedTiles="(1 -1 0), (1 0 -1)"});
		Ddt.Run("Perp2, origin, 1 rotation", {.Tiles=Perp2, .Rotation=1, .ExpectedTiles="(1 0 -1), (0 1 -1)"});
	}

	{ // Test Translate()
		struct Data
		{
			TArray<FVulHexAddr> Tiles;
			FVulHexVector Translation;
			FString Expected;
		};

		auto Ddt = VulTest::DDT<Data>(this, "Translate", [](const VulTest::TestCase& TC, const Data& Data)
		{
			const auto Shape = FVulHexShape(Data.Tiles);

			TC.Equal(Shape.Translate(Data.Translation).ToString(), Data.Expected);
		});

		Ddt.Run("Single hex (0, 0)", {.Tiles={{1, -1}}, .Translation={0, 0}, .Expected="(1 -1 0)"});
		Ddt.Run("Single hex (0, 1)", {.Tiles={{1, -1}}, .Translation={0, 1}, .Expected="(1 0 -1)"});

		const TArray<FVulHexAddr> Triangle = {{1, -1}, {1, 0}, {2, -1}};
		Ddt.Run("Triangle (0, 0)", {.Tiles=Triangle, .Translation={0, 0}, .Expected="(1 -1 0), (1 0 -1), (2 -1 -1)"});
		Ddt.Run("Triangle (1, 1)", {.Tiles=Triangle, .Translation={1, 1}, .Expected="(2 0 -2), (2 1 -3), (3 0 -3)"});
	}

	return !HasAnyErrors();
}
