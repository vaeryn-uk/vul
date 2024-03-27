#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "Components/Button.h"
#include "VulCollapsedPanel.generated.h"

/**
 * A widget which shows or hides its contents based on a connected button.
 */
UCLASS()
class VULRUNTIME_API UVulCollapsedPanel : public UContentWidget
{
	GENERATED_BODY()

public:
	/**
	 * Should the attached widget be initially open or closed?
	 */
	UPROPERTY(EditAnywhere)
	bool bStartOpen = false;

	/**
	 * The button that triggers the opening or closing of the panel when it is clicked.
	 *
	 * Must be an implementation of UCommonButtonBase. UButton support is not available.
	 *
	 * If not set, this panel cannot be toggled.
	 */
	UPROPERTY(EditAnywhere)
	UCommonButtonBase* Trigger;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ToggleContent(const TOptional<bool>& Force = {});

	bool bIsOpen = false;
};
