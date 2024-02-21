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
	for (const auto& [_, Widget, Hash, Controller] : Entries)
	{
		if (Hash.IsSet() && Widget.IsValid() && Controller.IsValid())
		{
			FVector2D Pos;
			Controller->GetMousePosition(Pos.X, Pos.Y);
			// TODO: This doesn't seem to update from rich text updates.
			Widget->SetPositionInViewport(Pos + VulRuntime::Settings()->TooltipMouseOffset);
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

	auto& [Contexts, Widget, Hash, _] = GetState(Controller);

	Contexts.Add(Context);

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

	Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UVulTooltipSubsystem::Hide(const FString& Context, const APlayerController* Controller) const
{
	auto& [Contexts, Widget, Hash, _] = GetState(Controller);

	Contexts.Remove(Context);

	if (Contexts.IsEmpty())
	{
		if (Widget.IsValid())
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}

		Hash.Reset();
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

UVulTooltipSubsystem* VulRuntime::Tooltip(const UObject* WorldCtx)
{
	return VulRuntime::WorldGlobals::GetGameInstanceSubsystemChecked<UVulTooltipSubsystem>(WorldCtx);
}
