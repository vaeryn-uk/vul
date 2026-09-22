#include "UserInterface/VulTextNotificationComponent.h"
#include "CommonUIEditorSettings.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "UserInterface/VulTextStyle.h"
#include "UserInterface/VulUserInterface.h"
#include "World/VulWorldGlobals.h"

DEFINE_VUL_LAZY_OBJ_PTR_SHORT(
	UVulTextNotificationComponent,
	Controller,
	VulRuntime::WorldGlobals::GetViewPlayerController(this)
);

UVulTextNotificationComponent::UVulTextNotificationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UVulTextNotificationComponent::Add(const FVulTextNotification& Notification)
{
	Notifications.Add(Notification, GetWorld());
}

void UVulTextNotificationComponent::Add(const FText& Text)
{
	Notifications.Add(FVulTextNotification(Text, DefaultTextDuration), GetWorld());
}

void UVulTextNotificationComponent::RemoveAll()
{
	Notifications.RemoveAll();
}

void UVulTextNotificationComponent::DestroyComponent(bool bPromoteChildren)
{
	RemoveAll();

	Super::DestroyComponent(bPromoteChildren);
}

void UVulTextNotificationComponent::BeginPlay()
{
	Super::BeginPlay();

	Notifications = TVulNotificationCollection<FVulTextNotification, UVulRichTextBlock>(
		[this](const FVulTextNotification& Notification) -> UVulRichTextBlock*
		{
			if (!ResolveController())
			{
				return nullptr;
			}

			// Copied from UWidgetTree::ConstructWidget
			const auto Widget = NewObject<UVulRichTextBlock>(
				Controller.Get(),
				TextWidgetClass.LoadSynchronous(),
				NAME_None,
				RF_Transactional
			);

			Widget->SetText(Notification.Text);
			if (!TextStyle.IsNull())
			{
				FTextBlockStyle Style;
				TextStyle.LoadSynchronous()->GetDefaultObject<UVulTextStyle>()->ToTextBlockStyle(Style);
				Widget->SetDefaultTextStyle(Style);
			}

			VulRuntime::UserInterface::AttachRootUMG(Widget, Controller.Get(), ZOrder);

			// Hidden until the first successful UpdateFn position -- CalculateScreenPosition returns unset
			// while GetDesiredSize() is still zero (immediately after creation, before a Slate layout pass),
			// so without this the widget would flash at its default (0,0) slot for that first frame.
			Widget->SetVisibility(ESlateVisibility::Hidden);

			return Widget;
		},
		[this](const FVulTextNotification& Data, UVulRichTextBlock* Widget, float X)
		{
			if (!ResolveController())
			{
				return;
			}

			const auto Position = VulRuntime::UserInterface::CalculateScreenPosition(
				Widget,
				Controller.Get(),
				GetRenderLocation(),
				ScreenTransform * X,
				FVector2D(.5),
				true
			);

			if (Position.IsSet())
			{
				// Copied from UUserWidget::SetPositionInViewport.
				const auto GVS = GEngine->GetEngineSubsystem<UGameViewportSubsystem>();
				auto New = UGameViewportSubsystem::SetWidgetSlotPosition(
					GVS->GetWidgetSlot(Widget),
					Widget,
					Position.GetValue(),
					true
				);

				New.ZOrder = ZOrder;

				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

				Widget->SetText(Data.Text);

				// Notifications without their own StyleOverride fall back to the component's TextStyle,
				// re-applied here (not just at allocation) so a recycled widget -- matched by Ref -- always
				// reflects the current notification's style rather than whatever it last had.
				const auto& EffectiveStyle = !Data.StyleOverride.IsNull() ? Data.StyleOverride : TextStyle;
				if (!EffectiveStyle.IsNull())
				{
					FTextBlockStyle Style;
					EffectiveStyle.LoadSynchronous()->GetDefaultObject<UVulTextStyle>()->ToTextBlockStyle(Style);
					Widget->SetDefaultTextStyle(Style);
				}

				GVS->SetWidgetSlot(Widget, New);
			}
		}
	);
}

FVector UVulTextNotificationComponent::GetRenderLocation() const
{
	return GetComponentLocation();
}

void UVulTextNotificationComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Notifications.UpdateAll();
}
