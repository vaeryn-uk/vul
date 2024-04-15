#pragma once

#include "CoreMinimal.h"
#include "VulRichTextIconDefinition.h"
#include "VulRichTextTooltipWrapper.h"
#include "Components/Border.h"
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
	UPROPERTY(meta=(BindWidget))
	USizeBox* Size;

	UPROPERTY(meta=(BindWidget))
	UImage* Icon;

	UPROPERTY(meta=(BindWidget))
	UBorder* Border;

	virtual bool Initialize() override;

	virtual USizeBox* GetAutoSizeBox() override;

	virtual TOptional<float> GetAutoSizeAspectRatio() override;

	bool ApplyIcon(const FVulRichTextIconDefinition* Definition);

#if WITH_EDITORONLY_DATA
	/**
	 * The icon to preview when TestIcon is selected.
	 */
	UPROPERTY(EditAnywhere, Category="Test")
	FName TestIconRowName;

	/**
	 * Loads & previews the icon indicated by TestIconRowName.
	 */
	UFUNCTION(CallInEditor, Category="Test")
	void TestIcon();
#endif

	/**
	 * Override this to provide an fallback icon when no icon is found. Useful as a visual
	 * indication that an icon is missing or an icon rich text syntax is wrong.
	 *
	 * Default is to return nullptr, where no icon is rendered.
	 */
	virtual TObjectPtr<UObject> FallbackIcon() const;
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
