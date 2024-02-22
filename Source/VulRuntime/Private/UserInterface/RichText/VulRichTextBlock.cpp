﻿#include "UserInterface/RichText/VulRichTextBlock.h"
#include "VulRuntimeSettings.h"
#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlockDecorator.h"
#include "Reflection/VulReflection.h"
#include "CommonTextBlock.h"
#include "UserInterface/RichText/VulRichTextTooltipWrapper.h"

const TMap<FString, FString>& UVulRichTextBlock::StaticContent() const
{
	const static TMap<FString, FString> Empty = {};
	return Empty;
}

const TMap<FString, TSharedPtr<const FVulTooltipData>>& UVulRichTextBlock::StaticTooltips() const
{
	const static TMap<FString, TSharedPtr<const FVulTooltipData>> Empty = {};
	return Empty;
}

/**
 * Private definition to satisfy FRichTextDecorator API. Simply proxies through to our
 * text block's implementation.
 */
class FVulTooltipDecorator : public FRichTextDecorator
{
public:
	FVulTooltipDecorator(UVulRichTextBlock* InTextBlock) : FRichTextDecorator(InTextBlock), TextBlock(InTextBlock) {};

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
	{
		return RunParseResult.Name == "tt";
	}

	UVulRichTextBlock* TextBlock;

protected:
	virtual TSharedPtr<SWidget> CreateDecoratorWidget(
		const FTextRunInfo& RunInfo,
		const FTextBlockStyle& DefaultTextStyle) const override
	{
		return TextBlock->DecorateTooltip(RunInfo, DefaultTextStyle);
	}
};

void UVulRichTextBlock::CreateDecorators(TArray<TSharedRef<ITextDecorator>>& OutDecorators)
{
	Super::CreateDecorators(OutDecorators);

	OutDecorators.Add(MakeShared<FVulTooltipDecorator>(this));
}

const TArray<UVulRichTextBlock::FVulDynamicTooltipResolver>& UVulRichTextBlock::DynamicTooltips() const
{
	const static TArray<FVulDynamicTooltipResolver> Empty = {};
	return Empty;
}

FString UVulRichTextBlock::StaticContentMarker(const FString& Str)
{
	return FString::Printf(TEXT("%%static(%s)%%"), *Str);
}

TSharedPtr<SWidget> UVulRichTextBlock::DecorateTooltip(
	const FTextRunInfo& RunInfo,
	const FTextBlockStyle& InDefaultTextStyle)
{
	FVulRichTextDynamicData Tooltip;

	for (const auto& Entry : RunInfo.MetaData)
	{
		if (Entry.Key == "static")
		{
			if (StaticTooltips().Contains(Entry.Value))
			{
				Tooltip = FVulRichTextDynamicData(StaticTooltips()[Entry.Value]);
				break;
			}
		}
	}

	if (!Tooltip.HasSubtype<TSharedPtr<const FVulTooltipData>>())
	{
		for (const auto& Delegate : DynamicTooltips())
		{
			if (const auto Result = Delegate.Execute(this, RunInfo); Result.IsSet())
			{
				Tooltip = Result.GetValue();
				break;
			}
		}
	}

	TSharedPtr<SWidget> Widget;

	if (!Tooltip.HasSubtype<TSharedPtr<SWidget>>())
	{
		// If not a widget, create the default and apply the tooltip data to it.
		checkf(!VulRuntime::Settings()->RichTextTooltipWrapper.IsNull(), TEXT("No rich text tooltip wrapper widget configured"))
		const auto Umg = CreateWidget<UVulRichTextTooltipWrapper>(this, VulRuntime::Settings()->RichTextTooltipWrapper.LoadSynchronous());
		checkf(IsValid(Umg), TEXT("Failed to create default rich text tooltip widget"))

		if (GetDefaultTextStyleClass())
		{
			// Pass through commonUI default text style class override if specified.
			// Have to hack with reflection here because this can only be changed in the
			// editor, but not CPP for some reason.
			FVulReflection::SetPropertyValue(
				Umg->Content,
				FName("DefaultTextStyleOverrideClass"),
				GetDefaultTextStyleClass()
			);
		}

		// Use the tooltip data specified above, if set, otherwise no tooltip is fine too (just no tooltip will appear).
		const auto Data = Tooltip.HasSubtype<TSharedPtr<const FVulTooltipData>>()
			? Tooltip.GetSubtype<TSharedPtr<const FVulTooltipData>>()
			: nullptr;

		Umg->VulInit(Data, RunInfo.Content);
		Widget = Umg->TakeWidget();
	} else
	{
		Widget = Tooltip.GetSubtype<TSharedPtr<SWidget>>();
	}

	return Widget;
}

FText UVulRichTextBlock::ApplyStaticSubstitutions(const FText& InText) const
{
	// This implementation feels heavy with all this string searching.
	// Might be a candidate for perf changes in the future.

	// TODO: Current implementation replaces the text in the editor. Would be nice for UMG editing
	//       to save the pre-substituted value.

	// TODO: Does this handling mess with FText formatting/localization? Need to test this.

	auto Replaced = false;
	// Do replacements on a Str to save a bunch of conversions between FString and FText.
	auto Str = InText.ToString();

	for (const auto& Entry : StaticContent())
	{
		if (Str.Find(Entry.Key) != INDEX_NONE)
		{
			Replaced = true;
			Str = Str.Replace(*Entry.Key, *Entry.Value);
		}
	}

	if (!Replaced)
	{
		// If nothing modified, use the untouched FText.
		return InText;
	}

	return FText::FromString(Str);
}

void UVulRichTextBlock::SetText(const FText& InText)
{
	Super::SetText(ApplyStaticSubstitutions(InText));
}

void UVulRichTextBlock::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	SetText(GetText()); // Trigger
}
