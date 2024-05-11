#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "UObject/Object.h"
#include "VulWrapButton.generated.h"

/**
 * A button that wraps a single child purely to provide click behaviour.
 *
 * This is an invisible widget and makes non-button elements click-able with
 * a button widget which has proper click-detection logic. This is better
 * solution that implementing OnNativeMouseDown or similar in your widget
 * directly.
 */
UCLASS()
class VULRUNTIME_API UVulWrapButton : public UButton
{
	GENERATED_BODY()

public:
	static UVulWrapButton* WrapWidget(UUserWidget* Owner, UWidget* ToWrap);

	DECLARE_MULTICAST_DELEGATE_OneParam(FVulWrapButtonClicked, UWidget*)

	/**
	 * Attach listeners here. This will be invoked with the widget this button
	 * wraps.
	 */
	FVulWrapButtonClicked WrapButtonClicked;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION()
	void TriggerWrapButtonClicked();
};
