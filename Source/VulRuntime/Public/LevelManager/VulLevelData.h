#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UObject/Object.h"
#include "VulLevelData.generated.h"

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
	 * Called when this level is shown (after loading is complete). You can use this to execute your
	 * own level-specific functionality.
	 */
	virtual void OnLevelShown();

	/**
	 * Returns a list of assets that will be loaded as part of this level's loading.
	 *
	 * Loading will not complete (and a new level not shown) until all of these assets are loaded.
	 *
	 * Right now, no special consideration is given to this objects once they are loaded in terms
	 * of their lifetime.
	 * TODO: How do we ensure that assets are not garbage collected in a useful way?
	 */
	virtual TArray<FSoftObjectPath> AssetsToLoad();
};
