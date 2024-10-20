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

				Widget->SetText(Data.Text);

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
