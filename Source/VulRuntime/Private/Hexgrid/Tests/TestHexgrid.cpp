#include "TestCase.h"
#include "Hexgrid/VulHexgrid.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestHexgrid,
	"VulRuntime.Hexgrid.TestHexgrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

using namespace VulTest;

typedef TVulHexgrid<FString> TestGrid;

TestGrid MakeGrid(int Size);
void TestTileDistance(TestHexgrid* TestCase, const FVulHexAddr& One, const FVulHexAddr& Two, const int Expected);
void TestConstruction(TestHexgrid* TestCase);
TestGrid::FPathResult<int> TestPath(
	TestHexgrid* TestCase,
	const int GridSize,
	const FVulHexAddr& Start,
	const FVulHexAddr& Goal,
	const int ExpectedLength,
	const TArray<FVulHexAddr>& Impassable = TArray<FVulHexAddr>(),
	const int ExpectedDistanceFromGoal = 0);

bool TestHexgrid::RunTest(const FString& Parameters)
{
	TestConstruction(this);

	TestTileDistance(this, FVulHexAddr(0, 0), FVulHexAddr(1, -1), 1);
	TestTileDistance(this, FVulHexAddr(0, 0), FVulHexAddr(3, -1), 3);
	TestTileDistance(this, FVulHexAddr(-2, 1), FVulHexAddr(1, 2), 4);
	TestTileDistance(this, FVulHexAddr(-3, 2), FVulHexAddr(3, -3), 6);
	TestTileDistance(this, FVulHexAddr(0, -2), FVulHexAddr(2, 1), 5);
	TestTileDistance(this, FVulHexAddr(-3, 0), FVulHexAddr(+3, 0), 6);

	{ // Adjacent.
		struct Data { FVulHexAddr To; int MaxRange; int GridSize; int ExpectedCount; };
		auto Ddt = VulTest::DDT<Data>(this, "Adjacent", [](TC TC, Data Data)
		{
			const auto Grid = MakeGrid(Data.GridSize);

			TC.Equal(Grid.AdjacentTiles(Data.To, Data.MaxRange).Num(), Data.ExpectedCount);
		});

		Ddt.Run("Origin, 1 adj", {.To = {0, 0}, .MaxRange = 1, .GridSize = 5, .ExpectedCount = 6});
		Ddt.Run("Origin, 2 adj", {.To = {0, 0}, .MaxRange = 2, .GridSize = 5, .ExpectedCount = 18});
		Ddt.Run("Invalid tile", {.To = {3, -2}, .MaxRange = 2, .GridSize = 2, .ExpectedCount = 0});
		Ddt.Run("Edge tile, 1 adj", {.To = {3, -2}, .MaxRange = 1, .GridSize = 3, .ExpectedCount = 4});
		Ddt.Run("Edge tile, 2 adj", {.To = {0, 3}, .MaxRange = 2, .GridSize = 3, .ExpectedCount = 8});
	}

	// Direct path cases; all reach goal.
	TestPath(this, 3, FVulHexAddr(-2, 1), FVulHexAddr(3, -3), 5);
	TestPath(this, 3, FVulHexAddr(-3, 0), FVulHexAddr(3, 0), 6);
	TestPath(this, 3, FVulHexAddr(3, -2), FVulHexAddr(0, -3), 4);

	{ // Single tile.
		const auto Result = TestPath(this, 3, FVulHexAddr(0, 0), FVulHexAddr(1, -1), 1);
		TestEqual("single tile path tile", Result.Tiles[0].Addr.ToString(), "(1 -1 0)");
	}


	// Reaches goal around some obstructing tiles.
	TestPath(
		this,
		3,
		FVulHexAddr(0, -0),
		FVulHexAddr(2, -2),
		5,
		{FVulHexAddr(1, -1), FVulHexAddr(0, -1), FVulHexAddr(1, 0)}
	);

	// Path to goal is fully blocked. Get as close as we can.
	TestPath(
		this,
		3,
		FVulHexAddr(-2, 0),
		FVulHexAddr(3, 0),
		3,
		{FVulHexAddr(3, -1), FVulHexAddr(2, 0), FVulHexAddr(2, 1)},
		2
	);

	// Null path check. When From == To.
	TestPath(this, 3, FVulHexAddr(0, 0), FVulHexAddr(0, 0), 0);

	TestPath(
		this,
		5,
		FVulHexAddr(5, 0),
		FVulHexAddr(0, 5),
		5
	);

	{
		struct Data { int GridSize; FVulHexAddr From; FVulHexAddr To; TArray<FVulHexAddr> ExpectedTiles; bool ExpectedComplete; };
		auto Ddt = DDT<Data>(this, "Hexgrid Trace", [](const TestCase& TestCase, const Data& Data)
		{
			auto Grid = MakeGrid(Data.GridSize);
			const auto Result = Grid.Trace(Data.From, Data.To);
			TestCase.Equal(Result.Complete, Data.ExpectedComplete);
			TestCase.Equal(Result.Tiles, Data.ExpectedTiles);
		});

		Ddt.Run("1 tile", {3, FVulHexAddr(0, 0), FVulHexAddr(1, -1), {FVulHexAddr(0, 0), FVulHexAddr(1, -1)}, true});
		Ddt.Run("3 tiles, non straight", {3, {0, 0}, {2, -3}, {{0, 0}, {1, -1}, {1, -2}, {2, -3}}, true});
		Ddt.Run("3 tiles, straight", {3, {0, 0}, {3, 0}, {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, true});

		// Check sampling tolerates large grids.
		TArray<FVulHexAddr> Expected;
		for (auto I = -25; I <= 25; I++)
		{
			Expected.Add({I, 0});
		}

		Ddt.Run("50 tiles, straight", {50, {-25, 0}, {25, 0}, Expected, true});
	}

	return true;
}

TestGrid MakeGrid(int Size)
{
	return TestGrid(Size, [](const FVulHexAddr& Addr)
	{
		return Addr.ToString();
	});
}

void TestTileDistance(TestHexgrid* TestCase, const FVulHexAddr& One, const FVulHexAddr& Two, const int Expected)
{
	FString Assertion = FString::Printf(TEXT("%s -> %s = %d"), *One.ToString(), *Two.ToString(), Expected);
	TestCase->TestEqual(Assertion, One.Distance(Two), Expected);
}

TestGrid::FPathResult<int> TestPath(
	TestHexgrid* TestCase,
	const int GridSize,
	const FVulHexAddr& Start,
	const FVulHexAddr& Goal,
	const int ExpectedLength,
	const TArray<FVulHexAddr>& Impassable,
	const int ExpectedDistanceFromGoal)
{
	auto Grid = MakeGrid(GridSize);

	TestGrid::TVulQueryOptions<int> Opts;
	Opts.CostFn = [Impassable](const TestGrid::FVulTile& From, const TestGrid::FVulTile& To, TestGrid* Grid) -> TOptional<int>
	{
		if (Impassable.Contains(To.Addr))
		{
			return {};
		}

		return 1;
	};

	const auto Result = Grid.Path<int>(Start, Goal, Opts);

	TestCase->TestEqual("Path Found", Result.Complete, ExpectedDistanceFromGoal == 0);

	const auto Tiles = Result.Tiles;
	TestCase->TestEqual("Path Cost", Result.Cost, ExpectedLength);
	if (TestCase->TestEqual("Path Length", Tiles.Num(), ExpectedLength))
	{
		// Check all tiles along the path are adjacent.
		for (int I = 1; I < ExpectedLength; I++)
		{
			auto What = FString::Printf(TEXT("Path #%d"), I-1);
			TestCase->TestEqual("Path #%d", true, Tiles[I-1].Addr.AdjacentTo(Tiles[I].Addr));
		}
	}

	if (Result.Tiles.Num() > 0)
	{
		TestCase->TestEqual("Path End Distance", Tiles.Last().Addr.Distance(Goal), ExpectedDistanceFromGoal);
	}

	return Result;
}

void TestConstruction(TestHexgrid* TestCase)
{
	struct Data { int Size; int ExpectedTileCount; };
	auto Ddt = DDT<Data>(TestCase, TEXT("Construction"), [](TC Test, Data Case)
	{
		auto Grid = MakeGrid(Case.Size);

		Test.Equal(Grid.GetSize(), Case.Size, TEXT("Size"));
		Test.Equal(Grid.TileCount(), Case.ExpectedTileCount, TEXT("TileCount"));
	});

	Ddt.Run(TEXT("1"), {1, 7});
	Ddt.Run(TEXT("2"), {2, 19});
	Ddt.Run(TEXT("3"), {3, 37});
}
