﻿#include "Misc/VulVectorPath.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/VulMath.h"

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

TArray<FVector> FVulVectorPath::GetPoints() const
{
	return Points;
}

FVulVectorPath FVulVectorPath::Randomize(
	const FRandomStream& Stream,
	const FBox& Box,
	const bool First,
	const bool Last
) const {
	if (!IsValid())
	{
		return FVulVectorPath();
	}

	TArray<FVector> Out = {};

	if (!First)
	{
		Out.Add(Points[0]);
	}

	const auto StartIndex = First ? 0 : 1;
	const auto EndIndex = Last ? Points.Num() : Points.Num() - 1;

	for (auto Index = StartIndex; Index < EndIndex; ++Index)
	{
		Out.Add(Stream.RandPointInBox(Box.MoveTo(Points[Index])));
	}

	if (!Last)
	{
		Out.Add(Points.Last());
	}

	return FVulVectorPath(Out);
}

FVulVectorPath FVulVectorPath::Curve(const float TurnDegsPerWorldUnit, const FVulVectorPathCurveOptions& Options) const
{
	if (!IsValid())
	{
		return FVulVectorPath();
	}

	const auto Termination = Distance * Options.TerminationFactor;

	TArray OutPath = {Points[0]};

	// How long between each sample.
	const auto SampleLength = Distance / (Points.Num() - 1) / Options.Samples;

	// Maximum rotation possible in a single segment.
	const auto DegsPerSample = TurnDegsPerWorldUnit * SampleLength;

	// This index is incremented as we pass points along the line.
	auto CurrentIndex = 0;
	// The point we're at in the curved path. This is moved every sample.
	auto CurrentPosition = Points[CurrentIndex];

	// The point in the path we have most-recently passed.
	auto PreviousTarget = Points[CurrentIndex];

	// The current direction of our travel. Start with the starting direction of the path at alpha=0,
	// unless otherwise specified.
	auto CurrentDirection = Options.StartDirection.IsSet() ? Options.StartDirection.GetValue() : Direction(0);

	// Where we're heading towards.
	auto Target = Points[CurrentIndex + 1];

	// Once we cross this plane, we know we need to move towards the next point.
	auto TargetPlane = FPlane(Target, UKismetMathLibrary::FindLookAtRotation(PreviousTarget, Target).Vector());

	// Checked-against each loop. Saves having to recalculate this (but must be recalc'd when target changes).
	auto TargetPlaneDot = TargetPlane.PlaneDot(PreviousTarget) < 0;

	// Are we heading towards the final point in the path? If yes, we check for path termination.
	auto HeadedTowardsEnd = CurrentIndex >= Points.Num() - 2;

	// Tracks the total distance of the path as we go. Used to check if we need to bail.
	auto DistanceTravelled = 0;

	while (true)
	{
		// Calculate any turn we need to make.
		const auto RequiredDirection = UKismetMathLibrary::FindLookAtRotation(CurrentPosition, Target);
		const auto RequiredTurn = (RequiredDirection - CurrentDirection).GetNormalized();
		const auto RequiredDegs = RequiredTurn.Euler().Size();

		// Limit the turn to the maximum we're allowed for a single sample.
		auto ActualTurn = FRotator::MakeFromEuler(RequiredTurn.Euler().GetSafeNormal() * FMath::Min(DegsPerSample, RequiredDegs));

		// And customize the rotation if requested to.
		if (Options.AdjustRotation)
		{
			ActualTurn = Options.AdjustRotation(ActualTurn, RequiredTurn);
		}

		CurrentDirection += ActualTurn;

		// LastPosition is used for line-segment checking of termination.
		const auto LastPosition = CurrentPosition;

		CurrentPosition += CurrentDirection.RotateVector(FVector::ForwardVector * SampleLength);
		DistanceTravelled += SampleLength;

		if (DistanceTravelled > Options.MaxLengthFactor * Distance)
		{
			// The path has not reached the end within the requested factor.
			// Give up to avoid infinite loops.
			return FVulVectorPath();
		}

		if (HeadedTowardsEnd)
		{
			// Check if we pass close enough to the end by seeing if the line segment
			// that we've moved in this sample comes close enough to the termination point.
			const auto Closest = FVulMath::ClosestPointOnLineSegment(LastPosition, CurrentPosition, Points.Last());

			if ((Points.Last() - Closest).Size() <= Termination)
			{
				OutPath.Add(Points.Last());
				break;
			}

		// Not heading towards the last point. Check to see if we've passed the current target.
		} else if (TargetPlaneDot != TargetPlane.PlaneDot(CurrentPosition) < 0)
		{
			// Have passed the plane and there's more to go. Recalculate a new target,
			// and all other variables that depend on it (saving needing to calculate
			// these inside the sample loop.
			CurrentIndex++;
			PreviousTarget = Points[CurrentIndex];
			Target = Points[CurrentIndex + 1];
			TargetPlane = FPlane(Target, UKismetMathLibrary::FindLookAtRotation(PreviousTarget, Target).Vector());
			TargetPlaneDot = TargetPlane.PlaneDot(PreviousTarget) < 0;
			HeadedTowardsEnd = CurrentIndex >= Points.Num() - 2;
		}

		// Detect if we need to record a point for this single sample moved.
		// If we're looking in a straight line to the target, we don't need to record anything.
		// Only if we have turned does this sample count.
		// Note this is deliberately after we've calculated a new target, because when we travel
		// straight to a point, we want to capture that point as we're just about to turn.
		if (!(UKismetMathLibrary::FindLookAtRotation(CurrentPosition, Target) - CurrentDirection).IsNearlyZero())
		{
			OutPath.Add(CurrentPosition);
		}
	}

	return FVulVectorPath(OutPath);
}

FVulVectorPath FVulVectorPath::Chop(const float Start, const float End) const
{
	if (!IsValid())
	{
		return FVulVectorPath();
	}

	const auto StartDistance = Distance * FMath::Clamp(Start, 0, 1);
	const auto EndDistance = Distance * FMath::Clamp(End, 0, 1);
	auto Travelled = 0.f;
	TArray NewPoints = {Interpolate(Start)};

	for (auto I = 1; I < Points.Num(); ++I)
	{
		const auto Segment = (Points[I] - Points[I - 1]);

		if (Travelled > StartDistance && Travelled < EndDistance)
		{
			// Point is within chopped path. Just copy it.
			NewPoints.Add(Points[I - 1]);
			Travelled += Segment.Length();
			continue;
		}

		Travelled += Segment.Length();
	}

	NewPoints.Add(Interpolate(End));

	return FVulVectorPath(NewPoints);
}

FVulVectorPath FVulVectorPath::Simplify() const
{
	if (!IsValid())
	{
		return FVulVectorPath();
	}

	// We always start from the beginning.
	TArray Simplified = {Points[0]};

	// If any non-start/end point does not lie on a straight line, we include it in the
	// simplified path.
	for (auto Index = 1; Index < Points.Num() - 1; ++Index)
	{
		const auto Closest = FVulMath::ClosestPointOnLineSegment(Points[Index - 1], Points[Index + 1], Points[Index]);

		if (Closest.Equals(Points[Index]))
		{
			// Straight line. Skip it.
			continue;
		}

		Simplified.Add(Points[Index]);
	}

	// We always finish at the end.
	Simplified.Add(Points.Last());

	return FVulVectorPath(Simplified);
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

float FVulVectorPath::FinalDestinationAlpha(const float Alpha) const
{
	const auto PointIndex = LastPointIndex(Alpha);
	if (PointIndex < Points.Num() - 2)
	{
		return -1;
	}

	if (PointIndex >= Points.Num() - 1)
	{
		return 1;
	}

	return (Interpolate(Alpha) - Points[PointIndex]).Size() / (Points[PointIndex + 1] - Points[PointIndex]).Size();
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

FVulVectorPath FVulVectorPath::Translate(const FVector& By) const
{
	TArray<FVector> NewPoints;

	for (const auto& Point : Points)
	{
		NewPoints.Add(Point + By);
	}

	return FVulVectorPath(NewPoints);
}

FVulVectorPath FVulVectorPath::RelocateEnd(const FVector& NewEnd) const
{
	auto New = *this;
	New.Points[New.Points.Num() - 1] = NewEnd;
	return MoveTemp(New);
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

FTransform FVulPathMovement::Apply(
	const FTransform& Current,
	const TFunction<FRotator(const FRotator Calculated, const float Alpha)>& AdjustDirection
) const {
	auto Ret = Current;

	auto Alpha = Started.ClampedAlpha(Duration);

	if (MovementCurve != nullptr)
	{
		Alpha = MovementCurve(Alpha);
	}

	Ret.SetLocation(Path.Interpolate(Alpha));
	FRotator Direction = Path.Direction(Alpha);

	if (AdjustDirection != nullptr)
	{
		Direction = AdjustDirection(Direction, Alpha);
	}

	Ret.SetRotation(Direction.Quaternion());

	return Ret;
}

bool FVulPathMovement::IsComplete() const
{
	return Started.Alpha(Duration) >= 1;
}

float FVulPathMovement::GetDuration() const
{
	return Duration;
}

const FVulVectorPath& FVulPathMovement::GetPath() const
{
	return Path;
}
