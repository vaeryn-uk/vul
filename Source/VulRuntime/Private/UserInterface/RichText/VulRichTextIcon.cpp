#include "UserInterface/RichText/VulRichTextIcon.h"
#include "CommonUISettings.h"
#include "ICommonUIModule.h"
#include "VulRuntimeSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBoxSlot.h"
#include "DeveloperSettings/VulDeveloperSettings.h"

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

TObjectPtr<UObject> UVulRichTextIcon::FallbackIcon() const
{
	return nullptr;
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

	// Create a widget early to get a fallback icon if we support it.
	// If not, and this Umg is not used, it will be cleaned up by GC.
	const auto Umg = CreateWidget<UVulRichTextIcon>(Owner, VulRuntime::Settings()->IconWidget.LoadSynchronous());

	const auto Found = ICommonUIModule::GetSettings().GetRichTextData()->FindIcon(FName(RunInfo.MetaData["i"]));
	UObject* Icon;
	if (!Found || Found->ResourceObject.IsNull())
	{
		Icon = Umg->FallbackIcon();
	} else
	{
		Icon = Found->ResourceObject.LoadSynchronous();
	}

	if (!IsValid(Icon))
	{
		return TSharedPtr<SWidget>();
	}

	Umg->Icon->SetBrushResourceObject(Icon);

	IVulAutoSizedInlineWidget::ApplyAutoSizing(Umg, RunInfo, DefaultTextStyle);

	return Umg->TakeWidget();
}
