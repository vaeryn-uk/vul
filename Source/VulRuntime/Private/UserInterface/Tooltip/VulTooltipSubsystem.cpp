#include "UserInterface/Tooltip/VulTooltipSubsystem.h"
#include "VulRuntime.h"
#include "VulRuntimeSettings.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "World/VulWorldGlobals.h"

bool FVulTooltipAnchor::operator==(const FVulTooltipAnchor& Other) const
{
	return Other.Widget == Widget;
}

void UVulTooltipSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	bIsEnabled = VulRuntime::Settings()->IsTooltipEnabled();
}

void UVulTooltipSubsystem::Tick(float DeltaTime)
{
	// Update widget positions to track the mouse.
	for (const auto& [_, Widget, Hash, Controller, ExistingData, Anchor] : Entries)
	{
		if (Hash.IsSet() && Widget.IsValid() && Controller.IsValid())
		{
			FVector2D Pos;
			bool HavePosition;
			if (Anchor.IsSet() && Anchor->Widget.IsValid())
			{
				HavePosition = BestWidgetLocationForWidget(Anchor->Widget.Get(), Widget.Get(), Pos);
			} else
			{
				HavePosition = Controller->GetMousePosition(Pos.X, Pos.Y);
				if (HavePosition)
				{
					HavePosition = BestWidgetLocationForMouse(FVector2D(Pos), Widget.Get(), Pos);
				}
			}

			if (HavePosition)
			{
				Widget->SetPositionInViewport(Pos);
				Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}
}

bool UVulTooltipSubsystem::IsTickable() const
{
	return bIsEnabled;
}

void UVulTooltipSubsystem::Show(
	const FString& Context,
	APlayerController* Controller,
	TSharedPtr<const FVulTooltipData> Data,
	const TOptional<FVulTooltipAnchor>& InAnchor
) const {
	if (!bIsEnabled)
	{
		UE_LOG(LogVul, Warning, TEXT("Request to show Vul tooltip, but this feature is disabled. Check Vul settings"))
		return;
	}

	if (!Data.IsValid())
	{
		return;
	}

	auto& [Contexts, Widget, Hash, _, ExistingData, Anchor] = GetState(Controller);

	if (Contexts.IsEnabled() && ExistingData != nullptr)
	{
		if (Data->GetTooltipPriority() < ExistingData->GetTooltipPriority())
		{
			// Current widget has a higher priority than the one being requested. Ignore.
			return;
		}

		if (Data->GetTooltipPriority() > ExistingData->GetTooltipPriority())
		{
			// If we're receiving an increased priority tooltip, clear any previous contexts
			// to ensure that when this higher-priority tooltip is later hidden, we don't use
			// a lower-priority context to keep this one open.
			Contexts.Reset();
		}
	}

	Contexts.Enable(Context);

	if (Hash.IsSet() && Hash.GetValue() == Data->Hash() && Anchor == InAnchor)
	{
		// Nothing's changed.
		return;
	}

	if (!Widget.IsValid())
	{
		Widget = CreateWidget(Controller, VulRuntime::Settings()->TooltipWidget.LoadSynchronous());
		checkf(Widget.IsValid(), TEXT("Could not load tooltip widget"))

		Widget->AddToPlayerScreen(VulRuntime::Settings()->TooltipZOrder);
	}
	

#if !USE_RTTI
	UE_LOG(LogVul, Error, TEXT("dynamic_cast usage invalid with no RTTI. Vul needs fixing for Android/Linux"))
	return;
#else
	const auto AsVulWidget = dynamic_cast<IVulTooltipWidget*>(Widget.Get());
	checkf(AsVulWidget, TEXT("Widget class does not implement IVulTooltipWidget"))
	AsVulWidget->Show(Data);
	Hash = Data->Hash();

	// Keep invisible until tick so that it doesn't appear until positioned correctly.
	Widget->SetVisibility(ESlateVisibility::Hidden);
	Widget->ForceLayoutPrepass();

	if (ExistingData)
	{
		// We're updating an existing tooltip so fire that the old one is hidden.
		OnDataHidden.Broadcast(ExistingData);
	}

	ExistingData = Data;
	Anchor = InAnchor;

	OnDataShown.Broadcast(Data, Widget.Get());
#endif
}

void UVulTooltipSubsystem::Hide(const FString& Context, const APlayerController* Controller) const
{
	auto& State = GetState(Controller);

	State.Contexts.Disable(Context);

	// Hash won't be set if we're already hidden.
	if (!State.Contexts.IsEnabled() && State.Hash.IsSet())
	{
		if (State.Widget.IsValid())
		{
			State.Widget->SetVisibility(ESlateVisibility::Collapsed);
		}

		OnDataHidden.Broadcast(State.Data);

		State.Hash.Reset();
		State.Data = nullptr;
	}
}

FText UVulTooltipSubsystem::PrepareCachedTooltip(const UObject* Widget, TSharedPtr<FVulTooltipData> Data)
{
	if (!Data.IsValid())
	{
		return FText::FromString("{content}");
	}

	if (CachedTooltips.Contains(Data->Hash()))
	{
		CachedTooltips.FindChecked(Data->Hash()).Value.Add(Widget);
	} else
	{
		TSet<TWeakObjectPtr<const UObject>> Objs;
		Objs.Add(Widget);
		CachedTooltips.Add(Data->Hash(), {Data, Objs});
	}

	GarbageCollectCachedTooltips();

	return FText::FromString(FString::Printf(TEXT("<tt cached=\"%s\">{content}</>"), *Data->Hash()));
}

FText UVulTooltipSubsystem::PrepareCachedTooltip(
	const UObject* Widget,
	TSharedPtr<FVulTooltipData> Data,
	const FText& Content
) {
	FFormatNamedArguments Args;
	Args.Add("content", Content);
	return FText::Format(PrepareCachedTooltip(Widget, Data), Args);
}

TSharedPtr<FVulTooltipData> UVulTooltipSubsystem::LookupCachedTooltip(const FString& Id)
{
	if (CachedTooltips.Contains(Id))
	{
		return CachedTooltips[Id].Key;
	}

	return nullptr;
}

UVulTooltipSubsystem::FWidgetState& UVulTooltipSubsystem::GetState(const APlayerController* Controller) const
{
	const auto PlayerIndex = Controller->GetLocalPlayer()->GetLocalPlayerIndex();

	if (Entries.IsValidIndex(PlayerIndex))
	{
		return Entries[PlayerIndex];
	}

	auto& Ref = Entries.InsertDefaulted_GetRef(PlayerIndex);
	Ref.Controller = Controller;
	return Ref;
}

bool UVulTooltipSubsystem::BestWidgetLocationForMouse(const FVector2D& For, const UWidget* Widget, FVector2D& Out) const
{
	auto Ctrl = Widget->GetOwningPlayer();
	if (!Ctrl)
	{
		return false;
	}

	UE::Math::TIntVector2<int32> ScreenSize;
	Ctrl->GetViewportSize(ScreenSize.X, ScreenSize.Y);

	const auto Offset = VulRuntime::Settings()->TooltipOffset;
	const auto WidgetSize = Widget->GetDesiredSize();

	if (For.X + WidgetSize.X + Offset.X > ScreenSize.X)
	{
		// Widget would overlap the right-hand side of the screen, so show on the left of For.
		Out.X = For.X - WidgetSize.X - Offset.X;
	} else
	{
		// Or we have room. Add the offset to the right.
		Out.X += Offset.X;
	}

	if (For.Y + WidgetSize.Y + Offset.Y > ScreenSize.Y)
	{
		// Widget would overlap bottom of the screen, so show above For.
		Out.Y = For.Y - WidgetSize.Y - Offset.Y;
	} else
	{
		// Or we have room. Add the offset below.
		Out.Y += Offset.Y;
	}

	return true;
}

bool UVulTooltipSubsystem::BestWidgetLocationForWidget(const UWidget* AnchorWidget, const UWidget* Tooltip, FVector2D& Out)
{
	auto Ctrl = Tooltip->GetOwningPlayer();
	if (!Ctrl)
	{
		return false;
	}

	UE::Math::TIntVector2<int32> ScreenSize;
	Ctrl->GetViewportSize(ScreenSize.X, ScreenSize.Y);

	const auto TooltipSize = Tooltip->GetDesiredSize();
	const auto HalfSizeX = FVector2D(TooltipSize.X / 2, 0);
	const auto TooltipOffset = VulRuntime::Settings()->TooltipOffset;

	if (auto Coords = GetWidgetScreenCoords(AnchorWidget, 0.5f, 0.0f); Coords.Y - TooltipSize.Y - TooltipOffset.Y > 0)
	{
		// Render above the anchor widget.
		Out = Coords - FVector2D(0, TooltipSize.Y) - FVector2D(0, TooltipOffset.Y) - HalfSizeX;
	} else if (Coords = GetWidgetScreenCoords(AnchorWidget, 0.5f, 1.0f); Coords.Y + TooltipSize.Y + TooltipOffset.Y < ScreenSize.Y)
	{
		// Render below the anchor widget.
		Out = Coords + FVector2D(0, TooltipOffset.Y) - HalfSizeX;
	} else
	{
		// TODO: check/render left-right if needed?
		return false;
	}

	return true;
}

FVector2D UVulTooltipSubsystem::GetWidgetScreenCoords(const UWidget* Widget, const float AnchorX, const float AnchorY)
{
	FVector2D Pixel;
	FVector2D Out;
	USlateBlueprintLibrary::AbsoluteToViewport(
		this,
		Widget->GetCachedGeometry().GetAbsolutePositionAtCoordinates(
			UE::Slate::FDeprecateVector2DParameter(AnchorX, AnchorY)
		),
		Out,
		Pixel
	);

	return Out;
}

void UVulTooltipSubsystem::GarbageCollectCachedTooltips()
{
	// TODO: Test this.
	TArray<FString> ToRemove;
	for (const auto& Entry : CachedTooltips)
	{
		// We remove unless any UObject reference is still valid.
		auto Remove = true;
		for (const auto& Obj : Entry.Value.Value)
		{
			if (Obj.IsValid())
			{
				Remove = false;
				break;
			}
		}

		if (Remove)
		{
			ToRemove.Add(Entry.Key);
		}
	}

	for (const auto& Key : ToRemove)
	{
		CachedTooltips.Remove(Key);
	}
}

UVulTooltipSubsystem* VulRuntime::Tooltip(const UObject* WorldCtx)
{
	return VulRuntime::WorldGlobals::GetGameInstanceSubsystemChecked<UVulTooltipSubsystem>(WorldCtx);
}

void VulRuntime::Tooltipify(
	const FString& Context,
	UWidget* Widget,
	FVulGetTooltipData Getter,
	const FVulTooltipWidgetOptions& Options
) {
	Widget->TakeWidget()->SetOnMouseEnter(FNoReplyPointerEventHandler::CreateWeakLambda(
		Widget,
		[Context, Getter, Widget, Options](const FGeometry&, const FPointerEvent&)
		{
			if (Options.OnShow)
			{
				Options.OnShow();
			}

			Tooltip(Widget)->Show(Context, Widget->GetOwningPlayer(), Getter.Execute(), Options.Anchor);
		}
	));

	Widget->TakeWidget()->SetOnMouseLeave(FSimpleNoReplyPointerEventHandler::CreateWeakLambda(
		Widget,
		[Context, Widget, Options](const FPointerEvent&)
		{
			if (Options.OnHide)
			{
				Options.OnHide();
			}

			Tooltip(Widget)->Hide(Context, Widget->GetOwningPlayer());
		}
	));
}

void VulRuntime::Tooltipify(const FString& Context, UWidget* Widget, TSharedPtr<const FVulTooltipData> Data)
{
	return Tooltipify(Context, Widget, FVulGetTooltipData::CreateWeakLambda(Widget, [Data] { return Data; }));
}
