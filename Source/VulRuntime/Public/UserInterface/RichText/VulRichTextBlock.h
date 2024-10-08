﻿#pragma once

#include "CoreMinimal.h"
#include "CommonRichTextBlock.h"
#include "Blueprint/UserWidget.h"
#include "CommonTextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "UserInterface/Tooltip/VulTooltip.h"
#include "Containers/Union.h"
#include "Framework/Text/ITextDecorator.h"
#include "VulRichTextBlock.generated.h"

/**
 * Extends common UI's rich text widget with extra functionality.
 *
 *   - Content substitutions: define content markers that are replaced with
 *     other content on render. Saves having to repeat common markup syntax for content
 *     that is used in multiple places throughout a game.
 *   - Retains CommonUI functionality for icons, which reads icons from the Vul setting's
 *     RichTextIcons data table.
 *   - An alternative for CommonUI's icon support that allows scaling of icons beyond the
 *     bounds of the surrounding text size.
 *
 * Syntax guide for text content:
 *
 *   - <vi i="icon_row_name" scale="custom_scale"/> for a Vul-rendered icon.
 *     See UVulRuntimeSettings::IconWidget.
 *     Note this is offered as an alternative to the CommonUI icon syntax:
 *     <ICON_NAME scale="XX">CONTENT</>
 *     ICON_NAME is the RowName of an icon defined in the Vul settings' RichTextIcons
 *     data table. Scale is an optional attribute to scale the icon; a value between 0
 *     and 1. Content is also optional, and is displayed based on the text block's inline
 *     setting and can render the content if there is room.
 *   - %content(NAME)%
 *     NAME matches on some key registered in this class and the whole string is
 *     replaced by that content if found. Implement this in child implementations'
 *     CreateStaticContent() and CreateDynamicContent() methods to add your own definitions
 *     (none are provided by default).
 *   - <tt static="STATIC_TOOLTIP_REF">CONTENT</>
 *     Renders a tooltip with statically-defined content. Implement StaticTooltips() to
 *     return tooltip data specific to your project. This integrates with UVulTooltipSubsystem
 *     to render tooltips for the player. CONTENT is rendered in nested rich text block, so
 *     CONTENT may include further rich text syntax. Note the text style of this nested rich
 *     text block is set to the style of the containing block.
 *   - <tt NAME="VALUE">CONTENT</>
 *     Similar to above, but for dynamic tooltips where your project has more control over the
 *     widget that is inlined in the text. Implement CreateDynamicTooltips() and provide delegates
 *     for how these are resolved.
 *   - <tt cached="[tooltip-id]">CONTENT</> will render pre-prepared tooltip data, saving having to
 *     pack all relevant data in to the rich text syntax then unpack & render in a decorator.
 *     See UVulTooltipSubsystem for using these.
 */
UCLASS()
class VULRUNTIME_API UVulRichTextBlock : public UCommonRichTextBlock
{
	GENERATED_BODY()

public:
	/**
	 * The result of a dynamic tooltip delegate, either:
	 *
	 * Widget is set, which is the widget to use to render the content inline. This is useful
	 * when a dynamic tooltip definition wants to entirely customize what is shown in line and
	 * also takes responsibility for triggering any tooltip (and the data for it). This widget
	 * can optionally implement IVulAutoSizedInlineWidget to have its size standarized by this
	 * rich text system and its scale controls.
	 *
	 * Or:
	 *
	 * The tooltip data to trigger when the rich text element is hovered. The default tooltip
	 * wrapper widget is used in this case, as per VulRuntime::Settings.
	 */
	typedef TUnion<UWidget*, TSharedPtr<const FVulTooltipData>> FVulRichTextDynamicData;

	/**
	 * Returns rich text tooltip data given the provided run info.
	 *
	 * This can either be:
	 *   - unset; we don't support the input markup.
	 *   - tool tip data; use the standard tooltip widget which displays the content as more rich text
	 *   - an SWidget instance; we're overriding the tooltip widget and are responsible for its tooltip logic (if any)
	 *
	 * The provided text block is the containing widget.
	 *
	 * The provided DefaultTextStyle can be used to assist with widget sizing.
	 */
	DECLARE_DELEGATE_RetVal_ThreeParams(
		TOptional<FVulRichTextDynamicData>,
		FVulDynamicTooltipResolver,
		UVulRichTextBlock*,
		const FRunInfo&,
		const FTextBlockStyle&
	);

	/**
	 * Convenience method to construct a Vul text block (or child class of).
	 */
	template <typename T = UVulRichTextBlock>
	static T* CreateTextBlock(
		UUserWidget* Owner,
		TSubclassOf<UCommonTextStyle> Style = nullptr,
		const FText& Content = FText::GetEmpty()
	) {
		const auto Spawned = Owner->WidgetTree->ConstructWidget<T>();

		if (Style != nullptr)
		{
			FTextBlockStyle StyleToApply;
			Style->GetDefaultObject<UCommonTextStyle>()->ToTextBlockStyle(StyleToApply);
			Spawned->SetDefaultTextStyle(StyleToApply);
		}

		Spawned->SetText(Content);

		return Spawned;
	}

	/**
	 * Utility function when defining static or dynamic content markets in your project.
	 *
	 * Returns %content(Str)% for your content keys. Not required, but useful
	 * to distinguish static content for authors.
	 */
	static FString ContentMarker(const FString& Str);

	/**
	 * Overwritten to ensure substitutions are parsed & replaced.
	 */
	virtual void SetText(const FText& InText) override;

protected:
	/**
	 * Returns static content substitutions available.
	 *
	 * This is separate to rich text support, in that it's simple string replacement whenever we render
	 * text. This is useful if you have frequently appearing text/markup, but only want to define it once.
	 *
	 * The keys in this map are recommended to include some unique text to distinguish this from normal
	 * content (and the rich text markup). StaticContentMarker can be used for this.
	 */
	virtual void CreateStaticContent(TMap<FString, FString>& Out) const {}

	/**
	 * Delegate to resolve dynamic content replacements. Provided the widget that
	 * is currently rendering the text being replaced. Must return true if content
	 * is available, and set OutString to that content. If this returns false, no
	 * replacement will take place.
	 */
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FVulRichTextDynamicContent, UWidget*, FString& OutString)
	/**
	 * Like CreateStaticContent, this provides simple string replacement for specific markers.
	 *
	 * However, instead of a fixed string, here you provide a delegate which returns the string
	 * to render on demand. This allows you to resolve content dynamically, such as based on
	 * the context in which the content is rendered.
	 */
	virtual void CreateDynamicContent(TMap<FString, FVulRichTextDynamicContent>&) const {};

	/**
	 * Returns static tooltip definitions, rendered via <tt static="NAME">CONTENT</>.
	 *
	 * NAME is the key in this map, and the returned tooltip data will be rendered when CONTENT
	 * is hovered in the game UI.
	 */
	virtual const TMap<FString, TSharedPtr<const FVulTooltipData>>& StaticTooltips() const;

	/**
	 * For more advanced tooltip resolution, where we need to generate tooltip data dynamically
	 * based on the parsed markup, or you want to provide a different content widget entirely.
	 *
	 * See FVulDynamicTooltipResolver.
	 */
	virtual void CreateDynamicTooltips(TArray<FVulDynamicTooltipResolver>& Tooltips) const;

	virtual void CreateDecorators(TArray<TSharedRef<ITextDecorator>>& OutDecorators) override;

	/**
	 * Overwritten to ensure substitutions are parsed & replaced.
	 */
	virtual void SynchronizeProperties() override;

private:
	friend class FVulTooltipDecorator;

	FText ApplyContentSubstitutions(const FText& Text, UWidget* Widget) const;

	/**
	 * Ensures the text we've set against the internal SWidget has static content substitutions applied.
	 *
	 * Not great as we need to call this in the correct places (highly coupled with parent), but can't
	 * find a more future-proof way of achieving a separation between the text set in the UWidget (e.g.
	 * UMG editor) and the substituted text we actually render.
	 */
	void ApplySWidgetText();

	TSharedPtr<SWidget> DecorateTooltip(const FTextRunInfo& RunInfo, const FTextBlockStyle& InDefaultTextStyle);

	const TArray<FVulDynamicTooltipResolver>& DynamicTooltips() const;
	const TMap<FString, FVulRichTextDynamicContent> DynamicContent() const;
	const TMap<FString, FString> StaticContent() const;
	mutable TOptional<TArray<FVulDynamicTooltipResolver>> CachedDynamicTooltips;
	mutable TOptional<TMap<FString, FVulRichTextDynamicContent>> CachedDynamicContent;
	mutable TOptional<TMap<FString, FString>> CachedStaticContent;
};