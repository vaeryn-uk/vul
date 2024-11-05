#pragma once

#include "CoreMinimal.h"
#include "LevelSequence.h"
#include "Components/Widget.h"
#include "UObject/Object.h"
#include "VulLevelData.generated.h"

/**
 * Describes a level that plays a level sequence (e.g. for cinematics).
 */
USTRUCT()
struct VULRUNTIME_API FVulSequenceLevelData
{
	GENERATED_BODY()

	/**
	 * A tag we look for in the just-loaded level for the first sequence actor to play.
	 *
	 * This allows you to create sequence actors in editor and have them found dynamically
	 * via a tag.
	 */
	UPROPERTY(EditAnywhere)
	FName LevelSequenceTag = NAME_None;

	/**
	 * Once the associated sequence is complete, we load this level (the value should match an entry
	 * in FVulLevelSettings).
	 */
	UPROPERTY(EditAnywhere)
	FName NextLevel;

	bool IsValid() const;
};

/**
 * Defines a UMG widget that will be automatically added to the viewport when a level is spawned.
 *
 * Widgets will be spawned in owned by the first player controller available.
 */
USTRUCT()
struct FVulLevelDataWidget
{
	GENERATED_BODY()

	/**
	 * The widget we'll create a new instance of and display.
	 *
	 * Note this is will be spawned and added to the first player controller's screen.
	 */
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<UUserWidget> Widget;

	/**
	 * The z-order the widget will be added with.
	 */
	UPROPERTY(EditAnywhere)
	int32 ZOrder = 0;
};

/**
 * Base definition of level data.
 *
 * You may extend this in your project to add additional data and provide
 * your own logic via the included virtual methods.
 *
 * This is intended to be extended first in CPP for any logic, then as
 * a blueprint to specify data in the editor.
 */
UCLASS(Blueprintable)
class VULRUNTIME_API UVulLevelData : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * The UE level object that will be loaded in & out.
	 */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UWorld> Level;

	/**
	 * If set, these widgets will be added to the viewport when this level starts.
	 */
	UPROPERTY(EditAnywhere)
	TArray<FVulLevelDataWidget> Widgets;

	/**
	 * Set this to make the level a cinematic level.
	 * If set, the sequence is automatically played, then the next level is loaded when complete.
	 */
	UPROPERTY(EditAnywhere)
	FVulSequenceLevelData SequenceSettings;

	/**
	 * Called when this level is shown (after loading is complete). You can use this to execute your
	 * own level-specific functionality.
	 */
	virtual void OnLevelShown(const struct FVulLevelShownInfo& Info);

	/**
	 * Adds to a list of assets that will be loaded as part of this level's loading.
	 *
	 * Loading will not complete (and a new level not shown) until all of these assets are loaded.
	 */
	virtual void AssetsToLoad(TArray<FSoftObjectPath>& Assets);

private:
	UFUNCTION()
	void OnSequenceFinished();

	UPROPERTY()
	class UVulLevelManager* LevelManager;
};
