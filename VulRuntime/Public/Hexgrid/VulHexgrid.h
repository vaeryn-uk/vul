﻿#pragma once

#include "CoreMinimal.h"
#include "Containers/VulPriorityQueue.h"
#include "UObject/Object.h"

/**
 * The address of a single tile in a 2D hexgrid.
 *
 * Uses a cube coordinate system.
 */
struct VULRUNTIME_API FVulHexAddr
{
	FVulHexAddr() = default; // Required for some containers.

	FVulHexAddr(const int InQ, const int InR) : Q(InQ), R(InR), S(-InR - InQ)
	{
		EnsureValid();
	}

	int Q;
	int R;
	int S;

	FString ToString() const;

	/**
	 * All the addresses that are adjacent to this address on a hexgrid.
	 *
	 * Note that the addresses returned may not be valid for a given grid due to its boundaries.
	 */
	TArray<FVulHexAddr> Adjacent() const;

	/**
	 * True if this tile is adjacent to (a neighbor of) Other.
	 */
	bool AdjacentTo(const FVulHexAddr& Other) const;

	/**
	 * Returns the distance between this and another grid address.
	 *
	 * As the crow flies.
	 */
	int Distance(const FVulHexAddr& Other) const;

	bool operator==(const FVulHexAddr& Other) const
	{
		return Other.Q == Q && Other.R == R && Other.S == S;
	}

	static TArray<int> GenerateSequenceForRing(const int Ring);

	bool IsValid() const;

private:
	void EnsureValid() const;
};

// For using as a key in maps.
FORCEINLINE uint32 GetTypeHash(const FVulHexAddr& Addr)
{
	return FCrc::MemCrc32(&Addr, sizeof(FVulHexAddr));
}

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
struct VULRUNTIME_API TVulHexgrid
{
	typedef TFunction<TileData (const FVulHexAddr& Addr)> FVulTileAllocator;

	struct VULRUNTIME_API TVulTile
	{
		TVulTile(const FVulHexAddr& InAddr, const TileData& InData) : Addr(InAddr), Data(InData) {};
		FVulHexAddr Addr;
		TileData Data;
	};

	TVulHexgrid() {}

	/**
	 * Creates a grid extending Size in positive and negative.
	 *
	 * Result is a hexagonal grid.
	 */
	explicit TVulHexgrid(const int Size, const FVulTileAllocator& Allocator)
	{
		checkf(Size > 0, TEXT("Hexgrid Size must be a greater than 0"))

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

	int Size() const { return (FMath::CeilToInt(FMath::Sqrt(static_cast<float>(TileCount()))) - 1) / 2; };
	int TileCount() const { return Tiles.Num(); };

private:

	TArray<TVulTile> AdjacentTiles(const FVulHexAddr& To) const
	{
		auto Addresses = To.Adjacent().FilterByPredicate([this](const FVulHexAddr& Adjacent)
		{
			return FMath::IsWithinInclusive(Adjacent.Q, Size() * -1, Size())
				&& FMath::IsWithinInclusive(Adjacent.R, Size() * -1, Size())
				&& FMath::IsWithinInclusive(Adjacent.S, Size() * -1, Size());
		});

		TArray<TVulTile> Out;

		for (auto Addr : Addresses)
		{
			Out.Add(Tiles.FindChecked(Addr));
		}

		return Out;
	}

	TMap<FVulHexAddr, TVulTile> Tiles;

	void AddTile(const FVulHexAddr& Addr, const FVulTileAllocator& Allocator)
	{
		Tiles.Add(Addr, TVulTile(Addr, Allocator(Addr)));
	}
};