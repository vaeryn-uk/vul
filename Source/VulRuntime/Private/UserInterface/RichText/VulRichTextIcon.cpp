﻿#include "UserInterface/RichText/VulRichTextIcon.h"
#include "VulRuntimeSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BorderSlot.h"
#include "Components/SizeBoxSlot.h"
#include "UserInterface/RichText/VulRichTextIconDefinition.h"
#if WITH_EDITORONLY_DATA
#include "VulEditorUtil.h"
#endif

bool UVulRichTextIcon::Initialize()
{
	const auto Ret = Super::Initialize();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("Size"));
		WidgetTree->RootWidget = Size;

		Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName("Outline"));
		const auto SizeBoxSlot = Size->SetContent(Border);
		Cast<USizeBoxSlot>(SizeBoxSlot)->SetPadding(FMargin(0));

		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName("Icon"));
		const auto BorderSlot = Border->SetContent(Icon);
		Cast<UBorderSlot>(BorderSlot)->SetPadding(FMargin(0));
	}

	return Ret;
}

USizeBox* UVulRichTextIcon::GetAutoSizeBox()
{
	return Size;
}

TOptional<float> UVulRichTextIcon::GetAutoSizeAspectRatio()
{
	// Assumes icons are 1:1.
	return 1;
}

bool UVulRichTextIcon::ApplyIcon(const FVulRichTextIconDefinition* Definition)
{
	UObject* IconToRender;
	FLinearColor Tint = FLinearColor::White;

	FSlateBrush BackgroundBrush;
	BackgroundBrush.TintColor = FLinearColor::Transparent;

	FMargin BackgroundPadding = FMargin(0);

	if (!Definition || Definition->ResourceObject.IsNull())
	{
		IconToRender = FallbackIcon();
	} else
	{
		IconToRender = Definition->ResourceObject.LoadSynchronous();
		Tint = Definition->Tint;

		if (Definition->ShowBackground)
		{
			BackgroundBrush = Definition->BackgroundBrush;
			BackgroundPadding = Definition->BackgroundPadding;
		}
	}

	if (!IsValid(IconToRender))
	{
		return false;
	}

	Icon->SetBrushResourceObject(IconToRender);
	Icon->SetBrushTintColor(Tint.ToFColor(false));
	Border->SetBrush(BackgroundBrush);
	Border->SetPadding(BackgroundPadding);

	return true;
}

void UVulRichTextIcon::TestIcon()
{
#if WITH_EDITORONLY_DATA
	if (TestIconRowName.IsNone())
	{
		FVulEditorUtil::Output("Icon Test", "No row name selected", EAppMsgCategory::Error);
		return;
	}

	const auto Found = VulRuntime::Settings()->ResolveIcon(TestIconRowName);

	if (Found == nullptr)
	{
		FVulEditorUtil::Output(
			"Icon Test",
			FString::Printf(TEXT("Could not find icon with row name=%s"), *TestIconRowName.ToString()),
			EAppMsgCategory::Error
		);
		return;
	}

	ApplyIcon(Found);
#endif
}

TObjectPtr<UObject> UVulRichTextIcon::FallbackIcon() const
{
	return nullptr;
}

bool FVulIconDecorator::Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const
{
	return RunParseResult.Name == "vi";
}

FString FVulIconDecorator::Markup(const FString& IconName)
{
	return FString::Printf(TEXT("<vi i=\"%s\"/>"), *IconName);
}

TSharedPtr<SWidget> FVulIconDecorator::CreateDecoratorWidget(
	const FTextRunInfo& RunInfo,
	const FTextBlockStyle& DefaultTextStyle
) const
{
	if (!RunInfo.MetaData.Contains("i"))
	{
		return TSharedPtr<SWidget>();
	}

	// Create a widget early to get a fallback icon if we support it.
	// If not, and this Umg is not used, it will be cleaned up by GC.
	const auto Umg = CreateWidget<UVulRichTextIcon>(Owner, VulRuntime::Settings()->IconWidget.LoadSynchronous());

	const auto Resolved = VulRuntime::Settings()->ResolveIcon(FName(RunInfo.MetaData["i"]));
	if (!Umg->ApplyIcon(Resolved))
	{
		return TSharedPtr<SWidget>();
	}

	IVulAutoSizedInlineWidget::ApplyAutoSizing(Umg, RunInfo, DefaultTextStyle);

	return Umg->TakeWidget();
}
