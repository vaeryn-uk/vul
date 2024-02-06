#pragma once

#include "CoreMinimal.h"
#include "Time/VulTime.h"
#include "UObject/Object.h"
#include "Components/Widget.h"
#include "VulNotification.generated.h"

/**
 * A UI notification is a generic structure to describe some text (or content) that's displayed
 * on the screen for a short period of time. This can be used to inform the player of some event
 * in realtime.
 *
 * This is a base class for different types of notifications defined below. Do not instantiate this
 * directly.
 *
 * Subclasses must implement operator==(const) const so that difference in a notification
 * can be detected, which triggers a re-render.
 */
USTRUCT(BlueprintType)
struct VULRUNTIME_API FVulUiNotification
{
	GENERATED_BODY()

	virtual ~FVulUiNotification() = default;

	/**
	 * The ref field is used for deduping. If set, the UI will replace any existing notifications that
	 * have a matching ref with this notification.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Ref;

	/**
	 * How long should this notification display for? In seconds.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RenderTime;

	bool operator==(const FVulUiNotification& Other) const;
};

/**
 * Static text content.
 */
USTRUCT(BlueprintType)
struct VULRUNTIME_API FVulTextNotification : public FVulUiNotification
{
	GENERATED_BODY()

	FVulTextNotification() = default;
	FVulTextNotification(const FText& InText, const float InRenderTime);
	FVulTextNotification(const FText& InText, const float InRenderTime, const FString& InRef);

	/**
	 * Text content of the notification.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Text;

	bool operator==(const FVulTextNotification& Other) const;
};

/**
 * Stores a single type of notification, providing timing and deduplication logic to always
 * provide an up-to-date, consistent set of notifications.
 *
 * We bind each notification with a UWidget, as specified in the template type. Consumers
 * must provide the following functions:
 *
 * * An allocator, which is responsible for returning a new WidgetType instance when a new
 *   notification is received. This is expected to add the created widget to the owning widget
 *   hierarchy in a container.
 * * An update fn, which is given the notification and pre-existing widget. It is expected to
 *   up date the widget according to the notification given. It is also given the amount we
 *   have progressed through its duration, as a figure between 0 (just started) and 1 (completed).
 *   Note that applying widget content should be applied here, and not in allocator, to ensure
 *   that when an existing widget gets new content, we apply it correctly.
 *
 * Note that deallocation is not customizable; we instead just remove a dead widget from its
 * parent when its notification is no longer required.
 *
 * This complexity is justified by the common use-case, and not having to reimplement widget-syncing
 * logic in various places and avoids having to recreate widgets which often leads to jumpy
 * interfaces.
 */
template <typename NotificationType, typename WidgetType>
struct TVulNotificationCollection
{
	static_assert(TIsDerivedFrom<NotificationType, FVulUiNotification>::Value, "NotificationType is not a valid notification");
	static_assert(TIsDerivedFrom<WidgetType, UWidget>::Value, "WidgetType is not a valid widget type");

	typedef TFunction<WidgetType* (const NotificationType& Notification)> FWidgetAllocator;
	typedef TFunction<void (const NotificationType& Notification, WidgetType*, float Completed)> FWidgetUpdater;

	struct FEntryStruct
	{
		NotificationType Notification;
		FVulTime Time;
		TWeakObjectPtr<WidgetType> Widget;
	};

	TVulNotificationCollection() = default;
	TVulNotificationCollection(
		FWidgetAllocator Allocator,
		FWidgetUpdater UpdateFn
	) : Allocator(Allocator), UpdateFn(UpdateFn) {};

	/**
	 * Checks for changes and expiry in contained notifications and returns true if there's been a change since the
	 * last call to this function.
	 *
	 * Designed to be driven by a Tick function on a UI component.
	 */
	bool CheckForChanges()
	{
		GC();

		if (Dirty)
		{
			Dirty = false;
			return true;
		}

		return Dirty;
	}

	/**
	 * Iterates all elements and invokes applicator on each with a valid widget.
	 *
	 * Intended for calls from a ticking context.
	 *
	 * Useful for text animations/location updates in the world.
	 */
	void UpdateAll()
	{
		GC(); // Drive removals.

		for (auto Entry : Entries)
		{
			if (Entry.Widget.IsValid())
			{
				UpdateFn(Entry.Notification, Entry.Widget.Get(), Entry.Time.Alpha(Entry.Notification.RenderTime));
			}
		}
	}

	/**
	 * Removes a notification by its identifying ref, if we have any in the collection.
	 *
	 * This allows you to remove a notification before it would naturally expire.
	 */
	void Remove(const FString& Ref)
	{
		if (Ref.IsEmpty())
		{
			return;
		}

		for (auto N = Entries.Num() - 1; N >= 0; --N)
		{
			if (Entries[N].Notification.Ref == Ref)
			{
				if (Entries[N].Widget.IsValid())
				{
					Entries[N].Widget.Get()->RemoveFromParent();
				}

				Dirty = true;
				Entries.RemoveAt(N);
			}
		}
	}

	/**
	 * Adds a notification to the collection, replacing any other notification that matches its
	 * ref and starts its expiry timer (which is why we need world).
	 */
	void Add(const NotificationType& Notification, UWorld* World)
	{
		auto Existing = -1;

		if (!Notification.Ref.IsEmpty())
		{
			// Look for an item to replace if we have a ref.
			Existing = Entries.IndexOfByPredicate([Notification](const FEntryStruct& Candidate)
			{
				return Notification.Ref == Candidate.Notification.Ref;
			});
		}

		FVulTime Time = FVulTime::WorldTime(World);

		if (Existing >= 0)
		{
			if (Entries[Existing].Notification == Notification)
			{
				// No changes.
				return;
			}

			// Replace if a ref match.
			if (Entries[Existing].Widget.IsValid())
			{
				UpdateFn(
					Notification,
					Entries[Existing].Widget.Get(),
					Entries[Existing].Time.Alpha(Entries[Existing].Notification.RenderTime)
				);

				Entries[Existing] = {Notification, Time, Entries[Existing].Widget};
			}
		} else
		{
			// Otherwise just append.
			auto Created = Allocator(Notification);
			UpdateFn(Notification, Created, Time.Alpha(Notification.RenderTime));
			Entries.Add({Notification, Time, Created});
		}

		Dirty = true;
	}

	/**
	 * Gets all active notifications in the order they should be displayed.
	 */
	const TArray<FEntryStruct>& Get() const
	{
		return Entries;
	}

private:
	TArray<FEntryStruct> Entries;

	/**
	 * Removes any timed out elements.
	 */
	void GC()
	{
		for (auto N = Entries.Num() - 1; N >= 0; --N)
		{
			if (Entries[N].Time.IsAfter(Entries[N].Notification.RenderTime))
			{
				if (Entries[N].Widget.IsValid())
				{
					Entries[N].Widget.Get()->RemoveFromParent();
				}

				Entries.RemoveAt(N);
				Dirty = true;
			}
		}
	}

	bool Dirty = false;

	FWidgetAllocator Allocator;
	FWidgetUpdater UpdateFn;
};
