#pragma once

#include "CoreMinimal.h"
#include "VulBorderStyleGenerator.generated.h"

/**
 * Defines a single variation to a multi border style.
 *
 * This is pretty much identical to a UVulMultiBorderStyle but can be edited inline.
 */
USTRUCT()
struct FVulBorderStyleVariation
{
	GENERATED_BODY()

	/**
	 * The borders that we will render. Brushes are rendered from first to last, so the final
	 * entry will be on top.
	 */
	UPROPERTY(EditAnywhere)
	TArray<FSlateBrush> Brushes;

	/**
	 * Padding that we apply between the borders and the widget's content.
	 */
	UPROPERTY(EditAnywhere)
	FMargin Padding;
};

/**
 * Generates a consistent set of multi border styles (UVulMultiBorderStyle).
 *
 * Unlike other generators, this does not have a template as the variations pretty much describe
 * an entire border style. This is still useful as a source of truth for all borders.
 */
UCLASS()
class VULEDITOR_API UVulBorderStyleGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * A style will be generated for each variation in this mapped, named based
	 * on the string key.
	 */
	UPROPERTY(EditAnywhere)
	TMap<FString, FVulBorderStyleVariation> Variations;

	/**
	 * Create or update all existing variations. Styles will be generated in the
	 * folder this generator belongs to.
	 */
	UFUNCTION(CallInEditor)
	void Generate();
};
