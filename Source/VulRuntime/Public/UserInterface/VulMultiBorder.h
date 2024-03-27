#pragma once

#include "CoreMinimal.h"
#include "Components/Border.h"
#include "VulMultiBorder.generated.h"

/**
 * A border that has reusable styles applied to it, similar to UCommonBorder.
 *
 * This varies, however, in that the styles may define multiple borders. Internally,
 * this widget overlays separate SBorder widgets on top of each other based on the
 * number of borders specified in the style.
 *
 * This is useful for a common scenario where a visual containing element is made up of
 * multiple textures, e.g. a boundary around the edge, then a background image.
 */
UCLASS()
class VULRUNTIME_API UVulMultiBorder : public UContentWidget
{
	GENERATED_BODY()

public:
	/**
	 * The multi border style to apply.
	 *
	 * Create a BP extending UVulMultiBorderStyle in the editor.
	 */
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<class UVulMultiBorderStyle> Style;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SBorder> MyRootBorder;

	static inline FSlateBrush DefaultBrush = FSlateBrush();

	TSharedRef<SBorder> CreateBorder(TAttribute<const FSlateBrush*> Brush);
};

/**
 * A style that is applied to a UVulMultiBorder.
 */
UCLASS(Blueprintable, Abstract)
class VULRUNTIME_API UVulMultiBorderStyle : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * The borders that we will render. Brushes are rendered from first to last, so the final
	 * entry will be on top.
	 */
	UPROPERTY(EditAnywhere)
	TArray<FSlateBrush> Brushes;

	/**
	 * Padding that we apply between the borders and the widget's content.
	 */
	UPROPERTY(EditAnywhere)
	FMargin Padding;
};
