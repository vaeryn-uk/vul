#include "UserInterface/Tooltip/VulTooltipSubsystem.h"
#include "VulRuntime.h"
#include "VulRuntimeSettings.h"
#include "Blueprint/UserWidget.h"
#include "World/VulWorldGlobals.h"

void UVulTooltipSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	bIsEnabled = VulRuntime::Settings()->IsTooltipEnabled();
}

void UVulTooltipSubsystem::Tick(float DeltaTime)
{
	// Update widget positions to track the mouse.
	for (const auto& [_, Widget, Hash, Controller, ExistingData] : Entries)
	{
		if (Hash.IsSet() && Widget.IsValid() && Controller.IsValid())
		{
			if (FVector2D Pos; Controller->GetMousePosition(Pos.X, Pos.Y))
			{
				Widget->SetPositionInViewport(BestWidgetLocation(Pos, Widget.Get()));
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
	TSharedPtr<const FVulTooltipData> Data) const
{
	if (!bIsEnabled)
	{
		UE_LOG(LogVul, Warning, TEXT("Request to show Vul tooltip, but this feature is disabled. Check Vul settings"))
		return;
	}

	auto& [Contexts, Widget, Hash, _, ExistingData] = GetState(Controller);

	Contexts.Enable(Context);

	if (Hash.IsSet() && Hash.GetValue() == Data->Hash())
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

	OnDataShown.Broadcast(Data, Widget.Get());
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

FVector2D UVulTooltipSubsystem::BestWidgetLocation(const FVector2D& For, const UWidget* Widget) const
{
	auto Ctrl = Widget->GetOwningPlayer();
	if (!Ctrl)
	{
		return For;
	}

	UE::Math::TIntVector2<int32> ScreenSize;
	Ctrl->GetViewportSize(ScreenSize.X, ScreenSize.Y);

	const auto Offset = VulRuntime::Settings()->TooltipMouseOffset;
	const auto WidgetSize = Widget->GetDesiredSize();
	auto Result = For;

	if (For.X + WidgetSize.X + Offset.X > ScreenSize.X)
	{
		// Widget would overlap the right-hand side of the screen, so show on the left of For.
		Result.X = For.X - WidgetSize.X - Offset.X;
	} else
	{
		// Or we have room. Add the offset to the right.
		Result.X += Offset.X;
	}

	if (For.Y + WidgetSize.Y + Offset.Y > ScreenSize.Y)
	{
		// Widget would overlap bottom of the screen, so show above For.
		Result.Y = For.Y - WidgetSize.Y - Offset.Y;
	} else
	{
		// Or we have room. Add the offset below.
		Result.Y += Offset.Y;
	}

	return Result;
}

UVulTooltipSubsystem* VulRuntime::Tooltip(const UObject* WorldCtx)
{
	return VulRuntime::WorldGlobals::GetGameInstanceSubsystemChecked<UVulTooltipSubsystem>(WorldCtx);
}
