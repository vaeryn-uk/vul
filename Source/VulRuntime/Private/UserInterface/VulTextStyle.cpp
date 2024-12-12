#include "UserInterface/VulTextStyle.h"

bool UVulTextStyle::ApplyTo(const TSoftClassPtr<UVulTextStyle>& Style, UVulRichTextBlock* TextBlock)
{
	if (IsValid(TextBlock) && !Style.IsNull())
	{
		FTextBlockStyle StyleToApply;
		Style.LoadSynchronous()->GetDefaultObject<UVulTextStyle>()->ToTextBlockStyle(StyleToApply);
		TextBlock->SetDefaultTextStyle(StyleToApply);

		return true;
	}

	return false;
}
