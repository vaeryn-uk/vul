#pragma once

#include "CoreMinimal.h"
#include "UserInterface/VulButtonStyle.h"
#include "VulButtonStyleGenerator.generated.h"

/**
 * Defines a single variation to a button style. This is
 * applied over the template specified in VulButtonStyleGenerator.
 *
 * Only background images are supported for now.
 */
USTRUCT()
struct FVulButtonStyleVariation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UTexture2D* NormalBackground = nullptr;

	UPROPERTY(EditAnywhere)
	UTexture2D* PressedBackground = nullptr;

	UPROPERTY(EditAnywhere)
	UTexture2D* HoveredBackground = nullptr;

	UPROPERTY(EditAnywhere)
	UTexture2D* DisabledBackground = nullptr;
};

/**
 * Generates button styles to produce a consistent set of varied buttons.
 *
 * After creating a new generator in the editor, you can edit its template
 * instance to serve as settings that are applied to all generated styles.
 * Then define your variations, which will create a style for each named variation
 * and apply the changes in the variation on top of the template.
 *
 * This functionality exists to save repetitive manual configuration across multiple
 * button styles and mitigates risk of inconsistent styling. The idea is that
 * you can make styling decisions and wrap them all up here in the template, then
 * use generate to update all derived styles as & when you need.
 */
UCLASS()
class VULEDITOR_API UVulButtonStyleGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Configure this instance to apply a base for all generated variations.
	 */
	UPROPERTY(Instanced, EditAnywhere)
	UVulButtonStyle* Template;

	/**
	 * A style will be generated for each variation in this mapped, named based
	 * on the string key.
	 */
	UPROPERTY(EditAnywhere)
	TMap<FString, FVulButtonStyleVariation> Variations;

	/**
	 * Create or update all existing variations. Styles will be generated in the
	 * folder this generator belongs to.
	 */
	UFUNCTION(CallInEditor)
	void Generate();
};
