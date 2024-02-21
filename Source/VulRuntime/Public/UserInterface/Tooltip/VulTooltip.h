#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulTooltip.generated.h"

/**
 * Content that is provided to a tooltip widget for rendering.
 *
 * You must provide your own UStruct implementation of this to integrate
 * with the Vul Tooltip system.
 *
 * An instance of your subclass will be provided to IVulTooltipWidget when
 * it is time to render the data in the widget.
 */
USTRUCT()
struct VULRUNTIME_API FVulTooltipData
{
	GENERATED_BODY()

	virtual ~FVulTooltipData() = default;
	virtual FString Hash() const PURE_VIRTUAL(, return "";);
};

/**
 * A widget must inherit this class to be used as a tooltip.
 *
 * You must implement RenderTooltip which can use GetData to access
 * the currently-displayed tooltip data.
 */
class VULRUNTIME_API IVulTooltipWidget
{
public:
	virtual ~IVulTooltipWidget() = default;
	void Show(TSharedPtr<const FVulTooltipData> Data);

protected:
	/**
	 * Gets the tooltip data to render. This is templated to get the tooltip data for your project.
	 */
	template <typename DataType, typename = TEnableIf<TIsDerivedFrom<DataType, FVulTooltipData>::Value>>
	const DataType* GetData() const
	{
		const auto Ret = dynamic_cast<const DataType*>(TooltipData.Get());
		checkf(Ret, TEXT("Could not convert tooltip data to requested type"))
		return Ret;
	}

	virtual void RenderTooltip() PURE_VIRTUAL();

private:
	TSharedPtr<const FVulTooltipData> TooltipData;
};
