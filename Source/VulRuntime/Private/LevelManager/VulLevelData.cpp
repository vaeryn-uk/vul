#include "LevelManager/VulLevelData.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "LevelManager/VulLevelManager.h"

bool FVulSequenceLevelData::IsValid() const
{
	return !LevelSequenceTag.IsNone();
}

void UVulLevelData::OnLevelShown(const FVulLevelShownInfo& Info, const FVulLevelEventContext& Ctx)
{
	LevelManager = Info.LevelManager;

	if (SequenceSettings.IsValid())
	{
		// Find the first actor that is a sequence with the matching tag.
		ALevelSequenceActor* SeqActor = nullptr;
		for (const auto& Actor : Info.ShownLevel->Actors)
		{
			SeqActor = Cast<ALevelSequenceActor>(Actor);
			if (IsValid(SeqActor) && SeqActor->Tags.Contains(SequenceSettings.LevelSequenceTag))
			{
				break;
			}
		}

		if (IsValid(SeqActor))
		{
			SeqActor->GetSequencePlayer()->OnFinished.AddUniqueDynamic(this, &UVulLevelData::OnSequenceFinished);
			SeqActor->GetSequencePlayer()->Play();
		}
	}
}

void UVulLevelData::AssetsToLoad(TArray<FSoftObjectPath>& Assets, const FVulLevelEventContext& Ctx)
{

}

void UVulLevelData::OnSequenceFinished()
{
	if (!SequenceSettings.NextLevel.IsNone() && IsValid(LevelManager))
	{
		LevelManager->LoadLevel(SequenceSettings.NextLevel);
	}
}
