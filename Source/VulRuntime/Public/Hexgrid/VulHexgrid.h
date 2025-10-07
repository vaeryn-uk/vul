#pragma once

#include "CoreMinimal.h"
#include "VulHexAddr.h"
#include "VulHexUtil.h"
#include "Containers/VulPriorityQueue.h"
#include "Misc/VulRngManager.h"
#include "UObject/Object.h"

/**
 * A 2D hexgrid using a cube-based 3D coordinate system, (q, r, s).
 *
 * https://www.redblobgames.com/grids/hexagons/#coordinates-cube
 *
 *                   ( 0 -2 +2)        (+1 -1 +1)        (+2 -2  0)
 *
 *          (-1 -1 +2)        ( 0 -1 +1)        (+1 -1  0)        (+2 -1 -1)
 *
 * (-2  0 +2)        (-1  0 +1)        ( 0  0  0)        (+1  0 -1)        (+2  0 -2)
 *
 *          (-2 +1 +1)        (-1 +1  0)        ( 0 +1 -1)        (+1 +1 -2)
 *
 *                   (-2 +2  0)        (-1 +2 -1)        ( 0 +2 -2)
 *
 * Templated to allow arbitrary data structures to be stored at each tile in the grid.
 */
template <typename TileData, typename CostType = int>
struct TVulHexgrid
{
	typedef TFunction<TileData (const FVulHexAddr& Addr)> FVulTileAllocator;

	struct FVulTile
	{
		FVulTile() = default;
		FVulTile(const FVulHexAddr& InAddr, const TileData& InData) : Addr(InAddr), Data(InData) {};
		FVulHexAddr Addr;
		TileData Data;
	};
	
	typedef TFunction<bool (const FVulTile&)> FVulTileValidFn;

	TVulHexgrid() = default;

	/**
	 * Creates a grid extending Size in positive and negative.
	 *
	 * Result is a hexagonal grid.
	 */
	explicit TVulHexgrid(const int InSize, const FVulTileAllocator& Allocator)
	{
		checkf(InSize > 0, TEXT("Hexgrid Size must be a greater than 0"))

		for (const auto& Addr : FVulHexAddr::GenerateGrid(InSize))
		{
			AddTile(Addr, Allocator);
		}

		Size = InSize;
	}

	/**
	 * Adds a tile to the grid, expanding the grid itself.
	 *
	 * Use in construction grid-building scenarios only.
	 * Use SetTileData to assign data to an existing grid.
	 */
	void AddTile(const FVulHexAddr& Addr, const TileData& Data)
	{
		Tiles.Add(Addr, FVulTile(Addr, Data));
	}

	/**
	 * Removes a tile from the grid.
	 */
	void RemoveTile(const FVulHexAddr& Addr)
	{
		Tiles.Remove(Addr);
	}

	/**
	 * Options we provide to @see Path to customize the path-finding algorithm.
	 */
	struct TVulQueryOptions
	{
		typedef TFunction<TOptional<CostType> (const FVulTile& From, const FVulTile& To, const TVulHexgrid* Grid)> FCostFn;
		typedef TFunction<CostType (const FVulHexAddr& From, const FVulHexAddr& To)> FHeuristicFn;
		
		static TOptional<CostType> DefaultCostFn(const FVulTile& From, const FVulTile& To, const TVulHexgrid* Grid)
		{
			return 1;
		}

		/**
		 * Returns the Euclidean distance between two tile addresses.
		 */
		static CostType DefaultHeuristic(const FVulHexAddr& From, const FVulHexAddr& To)
		{
			return From.Distance(To);
		}
		
		TVulQueryOptions(
			FCostFn InCostFn = &DefaultCostFn,
			FHeuristicFn InHeuristic = &DefaultHeuristic
		) : CostFn(InCostFn), Heuristic(InHeuristic) {}

		/**
		 * Given a tile From and its adjacent tile To, this function returns a cost to move between
		 * them.
		 *
		 * This can return unset to indicate that the movement is not valid.
		 */
		FCostFn CostFn = &DefaultCostFn;

		/**
		 * The heuristic that's used to estimate the cost to move between two (far) tiles.
		 * Our A* pathfinding uses this to guide which routes to check out next in its search.
		 */
		FHeuristicFn Heuristic = &DefaultHeuristic;
	};

	struct FTraceResult
	{
		/**
		 * The tiles along this trace, including start and end.
		 */
		TArray<FVulHexAddr> Tiles;

		/**
		 * If this trace made it to the requested destination without hitting an obstacle.
		 */
		bool Complete = false;

		/**
		 * How many tiles this trace covers excluding the start tile. This is effectively a range
		 * check. E.g. two adjacent tiles will have a trace distance of 1.
		 */
		int Distance() const { return FMath::Max(Tiles.Num() - 1, 0); }
	};

	/**
	 * Traces a straight line between From and To, returning the tiles that are underneath the trace.
	 *
	 * The optional function returns true if a tile is valid along the trace. Can be used to implement
	 * line of sight obstacles.
	 *
	 * Leeway allows the trace to deviate from the exact path by this amount of a single hex tile's size.
	 * This is useful when you want to prevent the trace being blocked when the trace navigates close
	 * to 2 tiles where one is blocked and one is not.
	 *
	 *           ( 0 -1 +1)        (+1 -1  0)
	 *
	 *  (-1  0 +1)        ( 0  0  0)        (+1  0 -1)
	 *
	 *  Consider finding a trace between ( 0 -1 +1) and (+1  0 -1). The trace will run along
	 *  the boundary between (+1 -1  0) and ( 0  0  0). This Leeway allows the trace to be
	 *  satisfied by either one of these tiles if either is blocked.
	 */
	FTraceResult Trace(
		const FVulHexAddr& From,
		const FVulHexAddr& To,
		const FVulTileValidFn& Check = [](const FVulTile&) { return true; },
		const float Leeway = 0.01
	) const {
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("VulHexgrid::Trace")

		FVulWorldHexGridSettings Settings;
		Settings.HexSize = 10;

		const auto Start = VulRuntime::Hexgrid::Project(From, Settings);
		const auto End = VulRuntime::Hexgrid::Project(To, Settings);
		const auto LineSegment = End - Start;
		const auto SampleCount = From.Distance(To);

		FTraceResult Result;
		Result.Tiles.Add(From);

		for (auto SampleN = 1; SampleN <= SampleCount; ++SampleN)
		{
			const auto Sample = Start + LineSegment * (SampleN / static_cast<float>(SampleCount));
			auto Tile = VulRuntime::Hexgrid::Deproject(Sample, Settings);

			if (!IsValidAddr(Tile))
			{
				return Result;
			}

			if (!Check(GetTile(Tile).GetValue()) && Leeway > 0.f)
			{
				const auto Sides = FVulMath::EitherSideOfLine(
					Start,
					End,
					SampleN / static_cast<float>(SampleCount),
					Settings.ProjectionPlane.GetNormal(),
					Settings.HexSize * Leeway
				);

				bool AlternateFound = false;

				for (const auto& Side : Sides)
				{
					const auto& Candidate = VulRuntime::Hexgrid::Deproject(Side, Settings);

					if (IsValidAddr(Candidate) && Check(GetTile(Candidate).GetValue()))
					{
						Tile = Candidate;
						AlternateFound = true;
						break;
					}
				}

				if (!AlternateFound)
				{
					return Result;
				}
			}

			Result.Tiles.Add(Tile);
		}

		Result.Complete = true;
		return Result;
	}

	/**
	 * Result of a @see Path call.
	 */
	struct FPathResult
	{
		/**
		 * Whether this path reaches the requested target.
		 */
		bool Complete;

		/**
		 * The tiles that make up the path in the tile grid.
		 *
		 * Note the starting tile is implied and not included here, but the To tile
		 * will be (assuming we have a complete path).
		 * This also means that for a null path query (where From == To), this will be empty.
		 *
		 * TODO: Make this const FVulTile* to save copying tile data?
		 */
		TArray<FVulTile> Tiles = {};

		TArray<FVulHexAddr> Addrs() const
		{
			TArray<FVulHexAddr> Out;

			for (const auto Tile : Tiles)
			{
				Out.Add(Tile.Addr);
			}

			return Out;
		}

		/**
		 * The cost of this path, according to the algorithm passed to our pathfinding.
		 */
		CostType Cost;
	};

	/**
	 * Generates Path query results for all eligible tiles that can be reached within MaxCost.
	 *
	 * If MaxCost is omitted, this will generate the shortest path data for all tiles that can
	 * be reached.
	 *
	 * This is a heavy call, but is more efficient than making separate Path queries for lots
	 * of tiles in the grid.
	 *
	 * Note that this only returns completed paths.
	 */
	TMap<FVulHexAddr, FPathResult> Paths(
		const FVulHexAddr& From,
		const TOptional<CostType> MaxCost = {},
		const TVulQueryOptions& Opts = TVulQueryOptions()
	)  const {
		TMap<FVulHexAddr, FPathResult> Result;

		TArray<TPair<FVulHexAddr, FPathResult>> WorkingSet;

		// Starting point. This is removed from the result set later.
		WorkingSet.Add({From, FPathResult{
			.Complete = true,
			.Tiles = {},
			.Cost = 0,
		}});

		int I = 0;
		while (I < WorkingSet.Num())
		{
			const auto Current = WorkingSet[I];

			for (const auto Next : AdjacentTiles(Current.Key))
			{
				if (Next.Addr == From)
				{
					// Not interested in routes that return to the origin tile.
					continue;
				}

				auto Cost = Opts.CostFn(
					Tiles.FindChecked(Current.Key),
					Tiles.FindChecked(Next.Addr),
					this
				);

				if (!Cost.IsSet())
				{
					continue;
				}

				if (MaxCost.IsSet() && Cost.GetValue() + Current.Value.Cost > MaxCost.GetValue())
				{
					continue;
				}

				auto NewResult = Current.Value;
				NewResult.Cost += Cost.GetValue();
				NewResult.Tiles.Add(Next);

				if (!Result.Contains(Next.Addr) || NewResult.Cost < Result[Next.Addr].Cost)
				{
					Result.Add(Next.Addr, NewResult);
					WorkingSet.Add({Next.Addr, NewResult});
				}
			}

			I++;
		}

		Result.Remove(From);

		return Result;
	}

	/**
	 * Finds a path between two tiles, From and To. Opts can be used to customize the path-finding.
	 *
	 * Returns one of the best possible paths.
	 *
	 * A* Search algorithm adapted from https://www.redblobgames.com/pathfinding/a-star/implementation.html#cpp-astar.
	 */
	FPathResult Path(
		const FVulHexAddr& From,
		const FVulHexAddr& To,
		const TVulQueryOptions& Opts = TVulQueryOptions()
	) const {
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("VulHexgrid::Path")

		if (From == To)
		{
			return {
				true,
				{},
				0
			};
		}

		struct FSearchNode
		{
			CostType Cost;
			FVulHexAddr Address;

			/**
			 * Used to select which node in the resulting data structure is closest.
			 */
			CostType RemainingEstimatedCost;
		};

		// All of the tiles that we've visited and the real cost to get that far.
		TMap<FVulHexAddr, FSearchNode> Visited = {{From, {0, From, Opts.Heuristic(From, To)}}};
		// The tiles on the edge of our search space thus far. With its estimated cost (euclidean distance to goal).
		// The node in this list with the lowest score is the next one we'll check.
		auto Frontier = TVulPriorityQueue<FVulHexAddr, CostType>();
		Frontier.Add(From, 0);

		while (!Frontier.IsEmpty())
		{
			auto Current = Frontier.Get().GetValue();

			if (Current.Element == To)
			{
				break;
			}

			for (auto Next : AdjacentTiles(Current.Element))
			{
				auto Cost = Opts.CostFn(
					Tiles.FindChecked(Current.Element),
					Tiles.FindChecked(Next.Addr),
					this
				);

				if (!Cost.IsSet())
				{
					continue;
				}

				const CostType NewCost = Visited.FindChecked(Current.Element).Cost + Cost.GetValue();

				if (!Visited.Contains(Next.Addr) || NewCost < Visited.FindChecked(Next.Addr).Cost)
				{
					const auto EstimatedCost = Opts.Heuristic(Next.Addr, To);
					Visited.Add(Next.Addr, FSearchNode{NewCost, Current.Element, EstimatedCost});
					Frontier.Add(Next.Addr, NewCost + EstimatedCost);
				}
			}
		}

		FPathResult Result;

		if (Visited.IsEmpty())
		{
			// Exhausted all paths.
			return Result;
		}

		// Grab a path with the lowest remaining estimated cost according to our heuristic.
		// For complete paths, this will generally be 0 (depending on the heuristic).
		TOptional<TPair<FVulHexAddr, FSearchNode>> Closest;

		for (auto Entry : Visited)
		{
			if (!Closest.IsSet() || Closest.GetValue().Value.RemainingEstimatedCost > Entry.Value.RemainingEstimatedCost)
			{
				Closest = Entry;
			}
		}

		// Walk the path in reverse back to the start point, building a path result along the way.
		auto Current = Closest.GetValue().Key;

		do
		{
			Result.Tiles.Add(Tiles.FindChecked(Current));
			Current = Visited[Current].Address;
		} while (Current != Visited[Current].Address);

		Result.Complete = Result.Tiles[0].Addr == To;
		Result.Cost = Visited[Closest->Key].Cost;

		// Put the tiles in the order of walking the path.
		Algo::Reverse(Result.Tiles);

		return Result;
	}

	/**
	 * Returns the size of this grid, that is the number of tiles from the center to an edge.
	 */
	int TileCount() const { return Tiles.Num(); };

	TArray<FVulTile> GetTiles() const
	{
		TArray<FVulTile> Out;
		Tiles.GenerateValueArray(Out);
		return Out;
	}

	/**
	 * Gets all the addresses that make up this grid.
	 */
	TArray<FVulHexAddr> GetTileAddrs() const
	{
		TArray<FVulHexAddr> Out;
		Tiles.GenerateKeyArray(Out);
		return Out;
	}

	TOptional<FVulTile> GetTile(const FVulHexAddr& Addr) const
	{
		if (!Tiles.Contains(Addr))
		{
			return {};
		}

		return *Tiles.Find(Addr);
	}

	TOptional<FVulTile> Find(const FVulHexAddr& Addr) const
	{
		if (!IsValidAddr(Addr))
		{
			return {};
		}

		return Tiles.FindChecked(Addr);
	}

	void SetTileData(const FVulHexAddr& Addr, const TileData& Data)
	{
		if (!ensureMsgf(IsValidAddr(Addr), TEXT("Cannot add to grid. Addr=%s is not valid for this grid"), *Addr.ToString()))
		{
			return;
		}

		// TODO: Do we destruct properly here?
		Tiles[Addr] = FVulTile(Addr, Data);
	}

	FVulTile* ModifyTileData(const FVulHexAddr& Addr)
	{
		if (!ensureMsgf(IsValidAddr(Addr), TEXT("Cannot modify grid data. Addr=%s is not valid for this grid"), *Addr.ToString()))
		{
			return nullptr;
		}

		return Tiles.Find(Addr);
	}

	bool IsValidAddr(const FVulHexAddr& Addr) const
	{
		return Tiles.Contains(Addr);
	}

	/**
	 * Returns the tiles adjacent to To, recursively searching for those tiles' adjacent tiles if the search if max range > 1.
	 *
	 * Return an empty array if the provided tile is not valid in this grid.
	 *
	 * Tiles closer to To will be returned first; farther tiles later.
	 */
	TArray<FVulTile> AdjacentTiles(const FVulHexAddr& To, const int MaxRange = 1, const bool IncludeStart = false) const
	{
		if (!IsValidAddr(To))
		{
			return {};
		}

		TArray<FVulTile> Out;

		for (const auto& Addr : FVulHexAddr::GenerateGrid(MaxRange))
		{
			const auto Translated = To.Translate(Addr.Vector());

			if (Translated == To && !IncludeStart)
			{
				continue;
			}

			if (IsValidAddr(Translated))
			{
				Out.Add(GetTile(Translated).GetValue());
			}
		}

		return Out;
	}

	/**
	 * Returns all tiles scored by ScoreFn then sorted by this score.
	 *
	 * ScoreFn may return unset if a tile should be excluded from the results.
	 */
	TArray<TPair<FVulTile, float>> ScoreTiles(
		const TFunction<TOptional<float> (const FVulTile&)>& ScoreFn,
		const bool Ascending = true
	) const;

	/**
	 * Splits the grid in two lists where each tile only appears in either list (or none).
	 *
	 * ValidFn filters tiles out entirely, returning false if a tile should not appear in either list.
	 * By default, all tiles are included.
	 * 
	 * SplitFn is responsible for partitioning each tile, returning true for the first list, and false for the second.
	 * By default, we split down the Q=0 axis (where Q=0 is in the first list).
	 */
	void SplitTiles(
		TArray<FVulHexAddr>& First,
		TArray<FVulHexAddr>& Second,
		const FVulTileValidFn& ValidFn = [](const FVulTile& Tile) { return true; },
		const TFunction<bool (const FVulTile&)>& SplitFn = [](const FVulTile& Tile) { return Tile.Addr.Q >= 0; }
	) const {
		for (const auto Tile : Tiles)
		{
			if (!ValidFn(Tile.Value))
			{
				continue;
			}
			
			if (SplitFn(Tile.Value))
			{
				First.Add(Tile.Key);
			} else
			{
				Second.Add(Tile.Key);
			}
		}
	}

private:

	int Size;
	TMap<FVulHexAddr, FVulTile> Tiles;

	void AddTile(const FVulHexAddr& Addr, const FVulTileAllocator& Allocator)
	{
		Tiles.Add(Addr, FVulTile(Addr, Allocator(Addr)));
	}
};

template <typename TileData, typename CostType>
TArray<TPair<typename TVulHexgrid<TileData, CostType>::FVulTile, float>> TVulHexgrid<TileData, CostType>::ScoreTiles(
	const TFunction<TOptional<float>(const FVulTile&)>& ScoreFn,
	const bool Ascending
) const {
	TArray<TPair<FVulTile, float>> Out;

	for (const auto& Tile : GetTiles())
	{
		if (const auto Score = ScoreFn(Tile); Score.IsSet())
		{
			Out.Add(TPair<FVulTile, float>(Tile, Score.GetValue()));
		}
	}

	Algo::Sort(Out, [Ascending](const TPair<FVulTile, float>& A, const TPair<FVulTile, float>& B) -> bool
	{
		if (FMath::IsNearlyEqual(A.Value, B.Value))
		{
			// Deterministic order for equal-scoring tiles.
			return A.Key.Addr.ToString() < B.Key.Addr.ToString();
		}

		return Ascending ^ (A.Value > B.Value);
	});

	return Out;
}
