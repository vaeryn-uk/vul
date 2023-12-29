#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 * A queue whose elements are ordered by some priority.
 *
 * This is not threadsafe.
 */
template <typename ElementType, typename PriorityType>
class VULRUNTIME_API TVulPriorityQueue
{
public:
	struct FEntry
	{
		ElementType Element;
		PriorityType Priority;

		bool operator<(const FEntry& Other) const
		{
			return Other.Priority < Other;
		}
	};

	/**
	 * Returns true if A is a higher priority than B (i.e. A comes out of the queue before B).
	 */
	typedef TFunction<bool (const PriorityType A, const PriorityType B)> Comparison;

	/**
	 * By default, lower values are higher priority.
	 */
	inline static Comparison DefaultComparison = [](const PriorityType A, const PriorityType B) { return A < B; };

	/**
	 * Creates a queue with the default priority order: lower = higher priority.
	 */
	TVulPriorityQueue()
	{
		Predicate.Comparison = DefaultComparison;
		Elements.Heapify(Predicate);
	}

	/**
	 * Creates a queue with a custom priority algorithm.
	 */
	explicit TVulPriorityQueue(Comparison Comparison)
	{
		Predicate.Comparison = Comparison;
		Elements.Heapify(Predicate);
	}

	/**
	 * Adds an element with the given priority.
	 */
	void Add(const ElementType& Element, const PriorityType Priority)
	{
		Elements.HeapPush({Element, Priority}, Predicate);
	}

	/**
	 * Gets and removes the lowest-priority element from the queue.
	 *
	 * Returns the the element and its priority as a TPair, or unset if there are no more elements.
	 */
	TOptional<FEntry> Get()
	{
		if (IsEmpty())
		{
			return {};
		}

		FEntry Out;
		Elements.HeapPop(Out, Predicate);
		return Out;
	}

	/**
	 * Returns true if there are no elements in the queue.
	 */
	bool IsEmpty() const
	{
		return Elements.Num() == 0;
	}
private:
	struct
	{
		Comparison Comparison;

		bool operator()(const FEntry& A, const FEntry& B) const
		{
			return Comparison(A.Priority, B.Priority);
		}
	} Predicate;

	TArray<FEntry> Elements;
};
