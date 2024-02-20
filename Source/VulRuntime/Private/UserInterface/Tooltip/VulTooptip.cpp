#include "UserInterface/Tooltip/VulTooptip.h"

void IVulTooltipWidget::Show(TSharedPtr<const FVulTooltipData> Data)
{
	TooltipData = Data;

	RenderTooltip();
}
