#pragma once

#include "CoreMinimal.h"
#include "Hexgrid/Addr.h"
#include "Containers/VulPriorityQueue.h"
#include "vector"
#include "UObject/Object.h"

template <typename S>
struct TTest
{
	TArray<S> Foo;
};

/**
 * A 2D hexgrid using a cube-based 3D coordinate system.
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

	struct TVulTile
	{
		TVulTile() = default;
		TVulTile(const FVulHexAddr& InAddr, const TileData& InData) : Addr(InAddr), Data(InData) {};
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
	struct FVulQueryOptions
	{
		static TOptional<CostType> DefaultCostFn(const TVulTile& From, const TVulTile& To)
		{
			return 1;
		}

		/**
		 * Given a tile From and its adjacent tile To, this function returns a cost to move between
		 * them.
		 *
		 * This can return unset to indicate that the movement is not valid.
		 */
		TFunction<TOptional<CostType> (const TVulTile& From, const TVulTile& To)> CostFn = &DefaultCostFn;

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
		 * Note the starting tile is implied and not included here.
		 * This also means that for a null path query (where From == To), this will be empty.
		 */
		TArray<TVulTile> Tiles;

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
		const FVulQueryOptions<CostType>& Opts = FVulQueryOptions<CostType>())
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
					Tiles.FindChecked(Next.Addr)
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

	TArray<TVulTile> GetTiles() const
	{
		TArray<TVulTile> Out;
		Tiles.GenerateValueArray(Out);
		return Out;
	}

	TOptional<TVulTile> Find(const FVulHexAddr& Addr) const
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
		Tiles[Addr] = TVulTile(Addr, Data);
	}

	bool IsValidAddr(const FVulHexAddr& Addr) const
	{
		return FMath::IsWithinInclusive(Addr.Q, Size * -1, Size)
			&& FMath::IsWithinInclusive(Addr.R, Size * -1, Size)
			&& FMath::IsWithinInclusive(Addr.S, Size * -1, Size);
	}

private:

	TArray<TVulTile> AdjacentTiles(const FVulHexAddr& To) const
	{
		auto Addresses = To.Adjacent().FilterByPredicate([this](const FVulHexAddr& Adjacent)
		{
			return IsValidAddr(Adjacent);
		});

		TArray<TVulTile> Out;

		for (auto Addr : Addresses)
		{
			Out.Add(Tiles.FindChecked(Addr));
		}

		return Out;
	}

	int Size;
	TMap<FVulHexAddr, TVulTile> Tiles;

	void AddTile(const FVulHexAddr& Addr, const FVulTileAllocator& Allocator)
	{
		Tiles.Add(Addr, TVulTile(Addr, Allocator(Addr)));
	}
};