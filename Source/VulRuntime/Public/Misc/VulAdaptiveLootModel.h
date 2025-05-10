#pragma once

#include "CoreMinimal.h"
#include "Misc/VulRngManager.h"
#include "UObject/Object.h"
#include "VulAdaptiveLootModel.h"

/**
 * Defines a single piece of loot stored in our loot model.
 *
 * DataType is your data identifying a piece of loot. Must supported use as a key in TMap.
 * 
 * TagsEnum is a list of valid tags that are used by the model; used to associate commonalities
 * between pieces of loot.
 *
 * TypeEnum is a list of different types of loot. Can be used to filter rolls.
 */
template <typename DataType, typename TagsEnum, typename TypeEnum>
class TVulAdaptiveLootData
{
public:
	/**
	 * The tags that are used to correlate this loot item with others in context.
	 */
	TArray<TagsEnum> Tags;
	
	/**
	 * The types that this loot belongs to. Used as a filter when rolling loot.
	 *
	 * E.g. some pieces of loot might be items, or buffs, or cards.
	 */
	TArray<TypeEnum> Types;

	/**
	 * The loot data, specific to your project.
	 */
	DataType Data;
};

/**
 * Provides loot rolls in a way that adapts to the loot the player has already attained.
 *
 * The idea is that as a player acquires loot throughout a run, they are more likely to
 * see related loot and increases the chances of piecing together a coherent build.
 *
 * This loot model operates on a pool and a context. The context is the loot that the player
 * has currently acquired, and the pool is the loot that can be rolled on.
 *
 * This API is designed to save on calculation needed for each roll. As the player rolls and
 * receives loot, changes must be reflected in this model to ensure that subsequent rolls are
 * accurate.
 *
 * A key design consideration here is the support for different types of loot. For example, when
 * rolling for cards, we obviously shouldn't be offering items or artifacts from our pool. However,
 * it is important to consider what items or artifacts the player has acquired in the roll for
 * cards. For this reason, a single model should encompass relevant different types of loot. When
 * rolling on cards, for example, we ensure that the pool is filtered down, but the context is still
 * relevant.
 *
 * The model itself is not responsible for providing decisions directly. This simply stores the data
 * that may be interesting to probability logic that you provide. This logic must assign a weight
 * value for each item, which defines how likely that item is to be drawn vs all others. This separation
 * allows you to use the same model to drive different loot logic implementations.
 */
template <typename DataType, typename TagsEnum, typename TypeEnum>
class TVulAdaptiveLootModel
{
public:
	using FLootData = TVulAdaptiveLootData<DataType, TagsEnum, TypeEnum>;
	
	struct FContextEntry
	{
		FLootData Data;
		int Amount;
	};

	struct FPoolEntry
	{
		FLootData Data;

		/**
		 * Number of tags in context that match a tag in this entry.
		 *
		 * If this is entry A and it has tags foo, bar and baz.
		 * Entry B has foo, bar
		 * Entry C has baz, qux.
		 *
		 * This will be 3 (2 from entry B and 1 from entry C).
		 */
		int CommonTags;

		/**
		 * The number of entries in context that have at least 1 tag in common with this one.
		 */
		int CommonEntries;
	};

	/**
	 * Adds an item to the pool of possible rewards.
	 *
	 * Does nothing if the item is already in the pool.
	 */
	void AddToPool(const FLootData& Item)
	{
		if (Pool.Contains(Item.Data))
		{
			return;
		}

		auto& Added = Pool.Add(Item.Data, FPoolEntry());
		Added.Data = Item;
		Recalculate(Added);
	}

	/**
	 * Adds an item to the model's context. I.e. the player has acquired this item.
	 */
	void AddToContext(const FLootData& Item)
	{
		InsertContext(Item);
		RecalculateAll();
	}

	/**
	 * Removes an item to the model's context. I.e. the player has lost this item.
	 */
	void RemoveFromContext(const FLootData& Item)
	{
		if (Context.Contains(Item.Data))
		{
			if (Context[Item.Data].Amount <= 1)
			{
				Context.Remove(Item.Data);
			} else
			{
				Context[Item.Data].Amount -= 1;
			}
		}
		
		RecalculateAll();
	}
	
	void AddToContext(const TArray<FLootData>& Items)
	{
		for (const auto& Item : Items)
		{
			InsertContext(Item);
		}

		RecalculateAll();
	}

	/**
	 * Called for each entry in the pool and returns a weight indicating
	 * how likely this entry is to be picked, relative to other entries'
	 * weights.
	 *
	 * If this entry should not be considered for any reason, return unset.
	 */
	using FWeightCalcFn = TFunction<TOptional<float> (const FPoolEntry&)>;

	/**
	 * Calculates weights for only items of a specific type of item in the model.
	 *
	 * The provided WeightCalc will only be called on items of the required type. This
	 * method is designed for the common case where you're offering rewards of a single
	 * type, e.g. cards the player can choose from (we wouldn't want to offer equipment,
	 * equipment will still matter for the weighting decisions).
	 *
	 * WeightCalc should return unset if the item is not available as reward for another
	 * reason.
	 *
	 * Normalized=true will return the weighted values in a 0-1 probability space.
	 */
	TMap<DataType, float> Weights(
		const FWeightCalcFn& WeightCalc,
		const bool Normalized = false,
		const TArray<TypeEnum>& Types = {}
	) const {
		TMap<DataType, float> Out;
		auto Total = 0;

		for (const auto& Entry : Pool)
		{
			if (!Types.IsEmpty() && !Entry.Value.Data.Types.ContainsByPredicate([Types](const TypeEnum& Candidate)
			{
				return Types.Contains(Candidate);
			}))
			{
				continue;
			}
				
			if (const auto Weight = WeightCalc(Entry.Value); Weight.IsSet())
			{
				Out.Add(Entry.Key, Weight.GetValue());
				Total += Weight.GetValue();
			}
		}

		if (Normalized && Total > 0)
		{
			TArray<DataType> Keys;
			Out.GetKeys(Keys);
			for (const auto& Key : Keys)
			{
				Out[Key] = Out[Key] / Total;
			}
		}

		return Out;
	}

	/**
	 * Picks a number of items from the provided weights calculation.
	 *
	 * Weights is by value to as we mutate the map for performance reasons.
	 * This usually comes straight out of a Weight function call, where the
	 * compiler will optimize this r-value to a move.
	 *
	 * These functions are separated to provide flexibility about how weights are
	 * calculated, and inspection of the calculated probabilities, e.g. for logging.
	 */
	TArray<DataType> Roll(
		const FVulRandomStream& Rng,
		TMap<DataType, float> Weights,
		const int Amount = 1
	) const {
		TArray<DataType> Out;

		if (Weights.IsEmpty())
		{
			return {};
		}

		for (auto I = 0; I < Amount; I++)
		{
			const auto Result = Rng.Weighted(Weights);
			if (!ensureAlwaysMsgf(Pool.Contains(Result), TEXT("Unknown result")))
			{
				return {};
			}
			
			Out.Add(Pool[Result].Data.Data);
			Weights.Remove(Result);
		}

		return Out;
	}

	/**
	 * Overload of Roll that calculates weights and then rolls them all in one.
	 */
	TArray<DataType> Roll(
		const FVulRandomStream& Rng,
		const FWeightCalcFn& WeightCalc,
		const int Amount = 1,
		const TArray<TypeEnum>& Types = {}
	) const {
		return Roll(Rng, Weights(WeightCalc, false, Types), Amount);
	}

private:
	void RecalculateAll()
	{
		for (auto& Entry : Pool)
		{
			Recalculate(Entry.Value);
		}
	}
	
	void Recalculate(FPoolEntry& PoolEntry)
	{
		PoolEntry.CommonEntries = 0;
		PoolEntry.CommonTags = 0;

		for (const auto& ContextEntry : Context)
		{
			int EntriesToAdd = 0;

			for (const auto& Tag : ContextEntry.Value.Data.Tags)
			{
				if (PoolEntry.Data.Tags.Contains(Tag))
				{
					if (EntriesToAdd == 0)
					{
						EntriesToAdd = ContextEntry.Value.Amount;
					}

					PoolEntry.CommonTags += ContextEntry.Value.Amount;
				}
			}

			PoolEntry.CommonEntries += EntriesToAdd;
		}
	}
	
	void InsertContext(const FLootData& Item)
	{
		if (Context.Contains(Item.Data))
		{
			Context[Item.Data].Amount += 1;
		} else
		{
			auto& Added = Context.Add(Item.Data, FContextEntry());
			Added.Data = Item;
			Added.Amount = 1;
		}
	}

	TMap<DataType, FPoolEntry> Pool;
	TMap<DataType, FContextEntry> Context;
};
