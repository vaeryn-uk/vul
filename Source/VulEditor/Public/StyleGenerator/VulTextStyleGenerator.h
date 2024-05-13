#pragma once

#include "CoreMinimal.h"
#include "VulTextStyle.h"
#include "UObject/Object.h"
#include "VulTextStyleGenerator.generated.h"

/**
 * Defines a single variation to a text style. This is
 * applied over the template specified in VulTextStyleGenerator.
 */
USTRUCT()
struct FVulTextStyleVariation
{
	GENERATED_BODY()

	/**
	 * The size of the font.
	 */
	UPROPERTY(EditAnywhere)
	float FontSize = 22.f;

	/**
	 * Size of the outline to apply.
	 */
	UPROPERTY(EditAnywhere)
	int32 OutlineSize = 0;

	UPROPERTY(EditAnywhere)
	bool bApplyColor = false;

	UPROPERTY(EditAnywhere)
	FLinearColor Color = FLinearColor::White;
};

/**
 * Generates a consistent set of text styles.
 */
UCLASS()
class VULEDITOR_API UVulTextStyleGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Configure this instance to apply a base for all generated variations.
	 */
	UPROPERTY(Instanced, EditAnywhere)
	UVulTextStyle* Template;

	/**
	 * A style will be generated for each variation in this mapped, named based
	 * on the string key.
	 */
	UPROPERTY(EditAnywhere)
	TMap<FString, FVulTextStyleVariation> Variations;

	/**
	 * Create or update all existing variations. Styles will be generated in the
	 * folder this generator belongs to.
	 */
	UFUNCTION(CallInEditor)
	void Generate();
};
