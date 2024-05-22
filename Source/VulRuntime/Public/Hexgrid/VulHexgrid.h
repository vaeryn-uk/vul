#pragma once

#include "CoreMinimal.h"
#include "VulHexAddr.h"
#include "VulHexUtil.h"
#include "Containers/VulPriorityQueue.h"
#include "vector"
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
template <typename TileData>
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

	TVulHexgrid() = default;

	/**
	 * Creates a grid extending Size in positive and negative.
	 *
	 * Result is a hexagonal grid.
	 */
	explicit TVulHexgrid(const int InSize, const FVulTileAllocator& Allocator)
	{
		checkf(InSize > 0, TEXT("Hexgrid Size must be a greater than 0"))
		Size = InSize;

		AddTile(FVulHexAddr(0, 0), Allocator);

		for (auto Ring = 1; Ring <= Size; Ring++)
		{
			const auto Seq = FVulHexAddr::GenerateSequenceForRing(Ring);

			auto Q = 0;
			auto R = Seq.Num() - Ring * 2;

			for (auto I = 0; I < Ring * 6; I++)
			{
				const FVulHexAddr Next(Seq[Q++ % Seq.Num()], Seq[R++ % Seq.Num()]);
				AddTile(Next, Allocator);
			}
		}
	}

	/**
	 * Options we provide to @see Path to customize the path-finding algorithm.
	 */
	template <typename CostType = int>
	struct TVulQueryOptions
	{
		static TOptional<CostType> DefaultCostFn(const FVulTile& From, const FVulTile& To, TVulHexgrid* Grid)
		{
			return 1;
		}

		typedef TFunction<TOptional<CostType> (const FVulTile& From, const FVulTile& To, TVulHexgrid* Grid)> FCostFn;

		/**
		 * Given a tile From and its adjacent tile To, this function returns a cost to move between
		 * them.
		 *
		 * This can return unset to indicate that the movement is not valid.
		 */
		FCostFn CostFn = &DefaultCostFn;

		/**
		 * Returns the euclidean distance between two tile addresses.
		 */
		static CostType DefaultHeuristic(const FVulHexAddr& From, const FVulHexAddr& To)
		{
			return From.Distance(To);
		}

		/**
		 * The heuristic that's used to estimate the cost to move between two (far) tiles.
		 * Our A* pathfinding uses this to guide which routes to check out next in its search.
		 */
		TFunction<CostType (const FVulHexAddr& From, const FVulHexAddr& To)> Heuristic = &DefaultHeuristic;
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
		const TFunction<bool (const FVulTile&)>& Check = [](const FVulTile&) { return true; },
		const float Leeway = 0.01
	) {
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
	template <typename CostType>
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
		 */
		TArray<FVulTile> Tiles;

		/**
		 * The cost of this path, according to the algorithm passed to our pathfinding.
		 */
		CostType Cost;
	};

	/**
	 * Finds a path between two tiles, From and To. Opts can be used to customize the path-finding.
	 *
	 * Returns one of the best possible paths.
	 *
	 * A* Search algorithm adapted from https://www.redblobgames.com/pathfinding/a-star/implementation.html#cpp-astar.
	 */
	template <typename CostType = int>
	FPathResult<CostType> Path(
		const FVulHexAddr& From,
		const FVulHexAddr& To,
		const TVulQueryOptions<CostType>& Opts = TVulQueryOptions<CostType>())
	{
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

		FPathResult<CostType> Result;

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
	int GetSize() const { return Size; }
	int TileCount() const { return Tiles.Num(); };

	TArray<FVulTile> GetTiles() const
	{
		TArray<FVulTile> Out;
		Tiles.GenerateValueArray(Out);
		return Out;
	}

	/**
	 * Gets all of the addresses that make up this grid.
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
		// TODO: Do we destruct properly here?
		Tiles[Addr] = FVulTile(Addr, Data);
	}

	FVulTile* ModifyTileData(const FVulHexAddr& Addr)
	{
		return Tiles.Find(Addr);
	}

	bool IsValidAddr(const FVulHexAddr& Addr) const
	{
		return FMath::IsWithinInclusive(Addr.Q, Size * -1, Size)
			&& FMath::IsWithinInclusive(Addr.R, Size * -1, Size)
			&& FMath::IsWithinInclusive(Addr.S, Size * -1, Size);
	}

	/**
	 * Returns the tiles adjacent to To, recursively searching for those tiles' adjacent tiles if the search if max range > 1.
	 *
	 * Return an empty array if the provided tile is not valid in this grid.
	 */
	TArray<FVulTile> AdjacentTiles(const FVulHexAddr& To, const int MaxRange = 1) const
	{
		if (!IsValidAddr(To))
		{
			return {};
		}

		TArray<FVulTile> Out;

		TArray NewTiles = {To};
		// Tracks our progress through NewTiles, which we just continue to add to as we iterate.
		auto NewTilesCheckIndex = 0;

		for (auto I = 0; I < MaxRange; I++)
		{
			const auto NewTileCount = NewTiles.Num() - NewTilesCheckIndex;

			for (auto TileIndex = NewTilesCheckIndex; TileIndex < NewTilesCheckIndex + NewTileCount; TileIndex++)
			{
				for (const auto& Adj : NewTiles[TileIndex].Adjacent())
				{
					// Valid in the grid and has not already been checked.
					if (IsValidAddr(Adj) && !NewTiles.Contains(Adj))
					{
						Out.Add(Tiles.FindChecked(Adj));
						NewTiles.Add(Adj);
					}
				}
			}

			NewTilesCheckIndex += NewTileCount;
		}

		return Out;
	}

private:

	int Size;
	TMap<FVulHexAddr, FVulTile> Tiles;

	void AddTile(const FVulHexAddr& Addr, const FVulTileAllocator& Allocator)
	{
		Tiles.Add(Addr, FVulTile(Addr, Allocator(Addr)));
	}
};