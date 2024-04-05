#include "UserInterface/RichText/VulRichTextIcon.h"
#include "CommonUISettings.h"
#include "ICommonUIModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBoxSlot.h"

bool UVulRichTextIcon::Initialize()
{
	const auto Ret = Super::Initialize();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName("Size"));
		WidgetTree->RootWidget = Size;
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName("Icon"));
		const auto SizeBoxSlot = Size->SetContent(Icon);
		Cast<USizeBoxSlot>(SizeBoxSlot)->SetPadding(FMargin(0));
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

bool FVulIconDecorator::Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const
{
	return RunParseResult.Name == "vi";
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

	const auto Found = ICommonUIModule::GetSettings().GetRichTextData()->FindIcon(FName(RunInfo.MetaData["i"]));
	if (!Found || Found->ResourceObject.IsNull())
	{
		return TSharedPtr<SWidget>();
	}

	const auto Umg = CreateWidget<UVulRichTextIcon>(Owner);
	Umg->Icon->SetBrushResourceObject(Found->ResourceObject.LoadSynchronous());

	IVulAutoSizedInlineWidget::ApplyAutoSizing(Umg, RunInfo, DefaultTextStyle);

	return Umg->TakeWidget();
}
