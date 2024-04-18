#include "Misc/VulVectorPath.h"
#include "Kismet/KismetMathLibrary.h"

FVector FVulVectorPath::Interpolate(const float Alpha) const
{
    if (!IsValid())
	{
		return FVector::Zero();
	}

	auto Remaining = Distance * FMath::Clamp(Alpha, 0, 1);

	for (auto I = 1; I < Points.Num(); ++I)
	{
		const auto Segment = (Points[I] - Points[I - 1]);

		if (Segment.Length() <= Remaining)
		{
			Remaining -= Segment.Length();
			continue;
		}

		return Points[I - 1] + Remaining / Segment.Length() * Segment;
	}

	return Points.Last();
}

float FVulVectorPath::GetDistance() const
{
	return Distance;
}

bool FVulVectorPath::IsValid() const
{
	// Need at least two points for this to be valid.
	return Points.Num() > 1;
}

void FVulVectorPath::CalculateDistance()
{
	Distance = 0;

	if (!IsValid())
	{
		return;
	}

	for (auto I = 1; I < Points.Num(); ++I)
	{
		const auto Prev = Points[I - 1];
		const auto Curr = Points[I];

		Distance += (Curr - Prev).Length();
	}
}

void FVulPathMovement::Apply(AActor* ToMove)
{
	if (IsComplete())
	{
		// Make sure we're at the end.
		ToMove->SetActorLocation(Path.Interpolate(1));
		return;
	}

	const auto New = Path.Interpolate(Started.Alpha(Duration));
	ToMove->SetActorLocationAndRotation(New, UKismetMathLibrary::FindLookAtRotation(ToMove->GetActorLocation(), New));
}

bool FVulPathMovement::IsComplete() const
{
	return Started.Alpha(Duration) >= 1;
}
