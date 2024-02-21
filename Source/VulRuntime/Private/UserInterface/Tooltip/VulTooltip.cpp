#include "UserInterface/Tooltip/VulTooltip.h"

void IVulTooltipWidget::Show(TSharedPtr<const FVulTooltipData> Data)
{
	TooltipData = Data;

	RenderTooltip();
}
