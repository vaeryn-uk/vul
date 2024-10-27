#pragma once

#include "CoreMinimal.h"
#include "LevelManager/VulLevelManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VulRuntimeSubsystem.generated.h"

/**
 * Exists for game instance lifetimes and initializes VUL functionality.
 */
UCLASS()
class VULRUNTIME_API UVulRuntimeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY()
	AVulLevelManager* LevelManager;
};
