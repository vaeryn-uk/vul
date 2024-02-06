#include "UserInterface/VulNotification.h"

bool FVulUiNotification::operator==(const FVulUiNotification& Other) const
{
	return Other.Ref == Ref;
}

bool FVulTextNotification::operator==(const FVulTextNotification& Other) const
{
	return FVulUiNotification(*this) == FVulUiNotification(Other) && Other.Text.EqualTo(Text, ETextComparisonLevel::Quinary);
}

FVulTextNotification::FVulTextNotification(const FText& InText, const float InRenderTime)
{
	Text = InText;
	Ref = FString();
	RenderTime = InRenderTime;
}

FVulTextNotification::FVulTextNotification(const FText& InText, const float InRenderTime, const FString& InRef)
{
	Text = InText;
	Ref = InRef;
	RenderTime = InRenderTime;
}
