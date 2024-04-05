#include "UserInterface/RichText/VulRichTextBlock.h"
#include "VulRuntimeSettings.h"
#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlockDecorator.h"
#include "Reflection/VulReflection.h"
#include "CommonTextBlock.h"
#include "UserInterface/RichText/VulRichTextTooltipWrapper.h"
#include "Widgets/Text/SRichTextBlock.h"

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
	OutDecorators.Add(MakeShared<FVulIconDecorator>(this));
}

void UVulRichTextBlock::CreateDynamicTooltips(TArray<FVulDynamicTooltipResolver>&) const
{

}

FString UVulRichTextBlock::ContentMarker(const FString& Str)
{
	return FString::Printf(TEXT("%%content(%s)%%"), *Str);
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
			if (const auto Result = Delegate.Execute(this, RunInfo, InDefaultTextStyle); Result.IsSet())
			{
				Tooltip = Result.GetValue();
				break;
			}
		}
	}

	UWidget* Widget;

	if (!Tooltip.HasSubtype<UWidget*>())
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

		Widget = Umg;
	} else
	{
		Widget = Tooltip.GetSubtype<UWidget*>();
	}

	// Apply scaling to the widget if specified.
	IVulAutoSizedInlineWidget::ApplyAutoSizing(Widget, RunInfo, InDefaultTextStyle);

	return Widget->TakeWidget();
}

const TArray<UVulRichTextBlock::FVulDynamicTooltipResolver>& UVulRichTextBlock::DynamicTooltips() const
{
	if (!CachedDynamicTooltips.IsSet())
	{
		CachedDynamicTooltips = TOptional<TArray<FVulDynamicTooltipResolver>>({});
		CreateDynamicTooltips(CachedDynamicTooltips.GetValue());
	}

	return CachedDynamicTooltips.GetValue();
}

const TMap<FString, UVulRichTextBlock::FVulRichTextDynamicContent> UVulRichTextBlock::DynamicContent() const
{
	if (!CachedDynamicContent.IsSet())
	{
		CachedDynamicContent = TMap<FString, FVulRichTextDynamicContent>();
		CreateDynamicContent(CachedDynamicContent.GetValue());
	}

	return CachedDynamicContent.GetValue();
}

const TMap<FString, FString> UVulRichTextBlock::StaticContent() const
{
	if (!CachedStaticContent.IsSet())
	{
		CachedStaticContent = TMap<FString, FString>();
		CreateStaticContent(CachedStaticContent.GetValue());
	}

	return CachedStaticContent.GetValue();
}

FText UVulRichTextBlock::ApplyContentSubstitutions(const FText& InText, UWidget* Widget) const
{
	// This implementation feels heavy with all this string searching.
	// Might be a candidate for perf changes in the future.

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

	for (const auto& Entry : DynamicContent())
	{
		if (Str.Find(Entry.Key) != INDEX_NONE)
		{
			if (FString New; Entry.Value.Execute(Widget, New))
			{
				Replaced = true;
				Str = Str.Replace(*Entry.Key, *New);
				break;
			}
		}
	}

	if (!Replaced)
	{
		// If nothing modified, use the untouched FText.
		return InText;
	}

	return FText::FromString(Str);
}

void UVulRichTextBlock::ApplySWidgetText()
{
	if (MyRichTextBlock.IsValid())
	{
		MyRichTextBlock->SetText(ApplyContentSubstitutions(GetText(), this));
	}
}

void UVulRichTextBlock::SetText(const FText& InText)
{
	Super::SetText(InText);
	ApplySWidgetText();
}

void UVulRichTextBlock::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplySWidgetText();
}
