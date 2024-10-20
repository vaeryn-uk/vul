#pragma once

#include "CoreMinimal.h"
#include "VulNotification.h"
#include "VulRuntime.h"
#include "Components/ActorComponent.h"
#include "RichText/VulRichTextBlock.h"
#include "VulTextNotificationComponent.generated.h"

/**
 * An Actor component that maintains UVulRichTextBlock notifications for an actor, rendering the text
 * in the game world at the component location.
 *
 * This is a scene component so it can be positioned relative to the owning actor, controlling where the
 * text is rendered.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VULRUNTIME_API UVulTextNotificationComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UVulTextNotificationComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<UVulRichTextBlock> TextWidgetClass = UVulRichTextBlock::StaticClass();

	/**
	 * Notifications will be adjusted linearly by this amount across their lifetime.
	 *
	 * XY screen coords between 0-1.
	 */
	UPROPERTY(EditAnywhere)
	FVector2D ScreenTransform = FVector2D::Zero();

	/**
	 * The Z-order that text is written to the viewport.
	 */
	UPROPERTY(EditAnywhere)
	int ZOrder = 0;

	/**
	 * The style applied to notifications.
	 */
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<UCommonTextStyle> TextStyle;

	/**
	 * The default seconds that we display text for. This can be overridden per call to Add, if needed.
	 */
	UPROPERTY(EditAnywhere)
	float DefaultTextDuration = 2.f;

	void Add(const FVulTextNotification& Notification);
	void Add(const FText& Text);

	void RemoveAll();

	virtual void DestroyComponent(bool bPromoteChildren = false) override;

protected:
	virtual void BeginPlay() override;

private:
	TVulNotificationCollection<FVulTextNotification, UVulRichTextBlock> Notifications;

	FVector GetRenderLocation() const;

	DECLARE_VUL_LAZY_OBJ_PTR(APlayerController, Controller);
};
