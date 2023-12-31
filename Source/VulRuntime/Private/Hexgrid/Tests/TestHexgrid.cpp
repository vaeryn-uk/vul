﻿#include "TestCase.h"
#include "Hexgrid/Hexgrid.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestHexgrid,
	"VulRuntime.Hexgrid.TestHexgrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

using namespace VulTest;

typedef TVulHexgrid<FString> TestGrid;

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

void TestPath(
	TestHexgrid* TestCase,
	const int GridSize,
	const FVulHexAddr& Start,
	const FVulHexAddr& Goal,
	const int ExpectedLength,
	const TArray<FVulHexAddr>& Impassable = TArray<FVulHexAddr>(),
	const int ExpectedDistanceFromGoal = 0)
{
	auto Grid = MakeGrid(GridSize);

	TestGrid::FVulQueryOptions<int> Opts;
	Opts.CostFn = [Impassable](const TestGrid::TVulTile& From, const TestGrid::TVulTile& To) -> TOptional<int>
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
	TestCase->TestEqual("Path Length", Tiles.Num(), ExpectedLength);
	TestCase->TestEqual("Path Cost", Result.Cost, ExpectedLength);

	for (int I = 1; I < ExpectedLength; I++)
	{
		auto What = FString::Printf(TEXT("Path #%d"), I-1);
		TestCase->TestEqual("Path #%d", true, Tiles[I-1].Addr.AdjacentTo(Tiles[I].Addr));
	}

	if (Result.Tiles.Num() > 0)
	{
		TestCase->TestEqual("Path End Distance", Tiles.Last().Addr.Distance(Goal), ExpectedDistanceFromGoal);
	}
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

bool TestHexgrid::RunTest(const FString& Parameters)
{
	TestConstruction(this);

	TestTileDistance(this, FVulHexAddr(0, 0), FVulHexAddr(1, -1), 1);
	TestTileDistance(this, FVulHexAddr(0, 0), FVulHexAddr(3, -1), 3);
	TestTileDistance(this, FVulHexAddr(-2, 1), FVulHexAddr(1, 2), 4);
	TestTileDistance(this, FVulHexAddr(-3, 2), FVulHexAddr(3, -3), 6);
	TestTileDistance(this, FVulHexAddr(0, -2), FVulHexAddr(2, 1), 5);
	TestTileDistance(this, FVulHexAddr(-3, 0), FVulHexAddr(+3, 0), 6);

	// Direct path cases; all reach goal.
	TestPath(this, 3, FVulHexAddr(-2, 1), FVulHexAddr(3, -3), 5);
	TestPath(this, 3, FVulHexAddr(-3, 0), FVulHexAddr(3, 0), 6);
	TestPath(this, 3, FVulHexAddr(3, -2), FVulHexAddr(0, -3), 4);

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

	return true;
}
