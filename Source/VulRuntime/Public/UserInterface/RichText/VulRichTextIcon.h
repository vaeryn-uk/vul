#pragma once

#include "CoreMinimal.h"
#include "VulRichTextTooltipWrapper.h"
#include "Components/Image.h"
#include "Components/RichTextBlockDecorator.h"
#include "VulRichTextIcon.generated.h"

/**
 * Renders an icon within rich text.
 *
 * This provides more control over the appearance of icons when rendered in rich text. Specifically, we
 * allow scaling greater than the surrounding text.
 *
 * TODO: This currently isn't suitable as a base class for widget blueprints as the BindWidget
 *       elements, created here, are not visible in the UMG widget tree, so the child widget won't compile.
 */
UCLASS(NotBlueprintable)
class VULRUNTIME_API UVulRichTextIcon : public UCommonUserWidget, public IVulAutoSizedInlineWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget), EditAnywhere)
	USizeBox* Size;

	UPROPERTY(meta=(BindWidget), EditAnywhere)
	UImage* Icon;

	virtual bool Initialize() override;

	virtual USizeBox* GetAutoSizeBox() override;

	virtual TOptional<float> GetAutoSizeAspectRatio() override;
};

class FVulIconDecorator : public FRichTextDecorator
{
public:
	FVulIconDecorator(URichTextBlock* InOwner)
		: FRichTextDecorator(InOwner) {}

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override;

protected:
	virtual TSharedPtr<SWidget> CreateDecoratorWidget(
		const FTextRunInfo& RunInfo,
		const FTextBlockStyle& DefaultTextStyle
	) const override;
};
