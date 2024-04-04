#include "UserInterface/Tooltip/VulTooltip.h"

int FVulTooltipData::GetTooltipPriority() const
{
	return 0;
}

void IVulTooltipWidget::Show(TSharedPtr<const FVulTooltipData> Data)
{
	TooltipData = Data;

	RenderTooltip();
}
