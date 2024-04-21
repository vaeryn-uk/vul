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

FVector FVulVectorPath::NextPoint(const float Alpha) const
{
	const int Index = LastPointIndex(Alpha);
	if (Index == INDEX_NONE)
	{
		return FVector::Zero();
	}

	return Points[FMath::Min(Index + 1, Points.Num() - 1)];
}

FRotator FVulVectorPath::Direction(const float Alpha) const
{
	const auto LastIndex = LastPointIndex(Alpha);

	if (LastIndex == INDEX_NONE)
	{
		return FRotator::ZeroRotator;
	}

	// Somewhere along the path. Look ahead to next point.
	if (Points.IsValidIndex(LastIndex + 1))
	{
		return UKismetMathLibrary::FindLookAtRotation(Points[LastIndex], Points[LastIndex + 1]);
	}

	// Must be at the end of the path. Rotate as if we've come from the last-but-one point.
	return UKismetMathLibrary::FindLookAtRotation(Points[Points.Num() - 2], Points[Points.Num() - 1]);
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

int FVulVectorPath::LastPointIndex(const float Alpha) const
{
	if (!IsValid())
	{
		return INDEX_NONE;
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

		return I - 1;
	}

	return Points.Num() - 1;
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

FTransform FVulPathMovement::Apply(const FTransform& Current)
{
	auto Ret = Current;

	const auto Alpha = Started.ClampedAlpha(Duration);

	Ret.SetLocation(Path.Interpolate(Alpha));
	Ret.SetRotation(Path.Direction(Alpha).Quaternion());

	LastAppliedAlpha = Alpha;

	return Ret;
}

FTransform FVulPathMovement::Apply(const FTransform& Current, const float RotationLimit)
{
	auto Ret = Current;
	if (IsComplete())
	{
		// Make sure we're at the end.
		Ret.SetLocation(Path.Interpolate(1));
		Ret.SetRotation(Path.Direction(1).Quaternion());
		return Ret;
	}

	const auto CurrentLoc = Current.GetLocation();
	const auto Alpha = Started.ClampedAlpha(Duration);
	const auto NextPoint = Path.NextPoint(Alpha);
	const auto IdealLoc = Path.Interpolate(Alpha);
	const auto PreviousIdealLoc = Path.Interpolate(LastAppliedAlpha);
	LastAppliedAlpha = Alpha;

	const auto IdealRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLoc, NextPoint);
	const auto IdealTurn = (IdealRotation - Current.GetRotation().Rotator()).GetNormalized().Euler();
	const auto RequiredDegs = IdealTurn.Size();
	const auto ActualRotAdjustment = IdealTurn.GetSafeNormal() * FMath::Min(RotationLimit, RequiredDegs);

	const auto NewRot = Current.GetRotation().Rotator() + FRotator::MakeFromEuler(ActualRotAdjustment);

	// Move forward based on the rotation we've calculated. This may take us directly along the path,
	// or we may be constrained by our turn amount.
	const auto Distance = (IdealLoc - PreviousIdealLoc).Size();
	const auto ActualMovement = NewRot.RotateVector(FVector(Distance, 0, 0));
	const auto NewLoc = CurrentLoc + ActualMovement;

	Ret.SetLocation(NewLoc);
	Ret.SetRotation(NewRot.Quaternion());

	return Ret;
}

bool FVulPathMovement::IsComplete() const
{
	return Started.Alpha(Duration) >= 1;
}
