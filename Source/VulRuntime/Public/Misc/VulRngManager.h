#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * Extends UE's native FRandomStream with some additional useful functionality.
 */
struct FVulRandomStream : FRandomStream
{
	/**
	 * Shuffles any contiguous container randomly.
	 *
	 * Copied from UE's Algo::RandomShuffle, but uses our stream's RNG instead.
	 */
	template <typename RangeType>
	void Shuffle(RangeType& Range) const
	{
		auto Data = GetData(Range);

		using SizeType = decltype(GetNum(Range));
		const SizeType Num = GetNum(Range);

		for (SizeType Index = 0; Index < Num - 1; ++Index)
		{
			// Get a random integer in [Index, Num)
			const SizeType RandomIndex = Index + (SizeType)RandHelper(Num - Index);
			if (RandomIndex != Index)
			{
				Swap(Data[Index], Data[RandomIndex]);
			}
		}
	}
};

/**
 * Provides streams to provide random number generation.
 *
 * Serves as a single location to access streams, intended to be the authority
 * on any randomness in your project.
 *
 * Streams are all managed by a single seed, such that a re-seeding will reset
 * all streams and provides per-stream determinism. Note that all streams are
 * deterministic when the manager is seeded, they are independently offset so
 * they produce different values.
 *
 * The design intention behind separate streams is to be able to make deterministic,
 * yet independent decisions. For example, a roguelike may have a seed settable for a
 * given run, but which combats are encountered should not be influenced by the
 * number of attack rolls a player made in a combat. In this scenario, you would have
 * one stream for attack rolls and another stream for encounter generation.
 *
 * The only exception to the seeding logic is the special SeedlessStream() which is
 * provided for convenience. This is useful when your RNG use-case is truly independent
 * of any seeding (and any of your project-specific streams). Note that this stream
 * and the default constructor's random seed are both beholden to FMath::SRand so can
 * be seeded globally if needed (e.g. for tests).
 *
 * To use this in your project, define a UENUM which specifies each of your streams.
 * Construct a new manager and pass in that enumtype, e.g.
 *
 *   UENUM()
 *   enum class EMyProjectRngStream : uint8
 *   {
 *       Game,
 *       Graphics,
 *       AI,
 *   }
 *
 *   auto RngManager = TVulRngManager<EMyProjectRngStream>();
 *   RngManager.Stream(EMyProjectRngStream::Game).GetUnsignedInt();
 *
 * This is intended to be single-instance, and should not be copied. The recommendation
 * is to initialize one of these in a game singleton/subsystem and access it from locations
 * where random numbers are needed.
 */
template <typename EnumType, typename = typename TEnableIf<TIsEnum<EnumType>::Value>::Type>
struct VULRUNTIME_API TVulRngManager
{
	/**
	 * Creates a new manager and seeds all streams with a random seed.
	 */
	TVulRngManager()
	{
		UEnum* Enum = StaticEnum<EnumType>();

		for (auto I = 0; I < Enum->NumEnums(); I++)
		{
			Entries.Add(static_cast<EnumType>(Enum->GetValueByIndex(I)), FVulStreamEntry{.Stream = FVulRandomStream(), .Offset = I});
		}

		Seed(RandomSeed());

		Seedless = FVulRandomStream();
		Seedless.Initialize(RandomNumber());
	}

	// Disable copying.
	TVulRngManager(const TVulRngManager&) = delete;
	TVulRngManager& operator=(const TVulRngManager&) = delete;

	/**
	 * Generates a random seed.
	 *
	 * Note: this random generation is unrelated to any seeding within this class.
	 */
	static FString RandomSeed()
	{
		// TODO: Something more human-memorable?
		return FString::Printf(TEXT("%X"), RandomNumber());
	}

	/**
	 * Seed all streams provided by this manager.
	 *
	 * For any given seed, from this point on, all streams produce deterministic results.
	 */
	void Seed(const FString& Seed)
	{
		CurrentSeed = Seed;
		const auto IntSeed = FCrc::StrCrc32(*Seed);

		for (auto& Entry : Entries)
		{
			Entry.Value.Stream.Initialize(IntSeed + Entry.Value.Offset);
		}
	}

	/**
	 * Gets the current seed used by this manager.
	 */
	FString GetSeed() const
	{
		return CurrentSeed;
	}

	/**
	 * Retrieves the requested stream.
	 */
	const FVulRandomStream& Stream(const EnumType Stream) const
	{
		checkf(Entries.Contains(Stream), TEXT("Cannot retrieve unrecognized RNG stream"));

		return Entries[Stream].Stream;
	}

	/**
	 * Access to a special stream that is independent of any other streams or seeding of this
	 * manager.
	 */
	const FVulRandomStream& SeedlessStream() const
	{
		return Seedless;
	}

private:
	struct FVulStreamEntry
	{
		FVulRandomStream Stream;
		int32 Offset;
	};

	/**
	 * When the manager needs a random value independent of seeded streams.
	 *
	 * Uses FMath::SRand, so can be controlled by FMath::SRandInit if needed.
	 */
	static int32 RandomNumber()
	{
		return FMath::SRand() * RAND_MAX;
	}

	TMap<EnumType, FVulStreamEntry> Entries;
	FString CurrentSeed;
	FVulRandomStream Seedless;
};
