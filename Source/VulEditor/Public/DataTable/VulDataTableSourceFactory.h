#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "VulDataTableSourceFactory.generated.h"

/**
 *
 */
UCLASS()
class VULEDITOR_API UVulDataTableSourceFactory : public UFactory
{
	GENERATED_BODY()

public:
	UVulDataTableSourceFactory();

	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;
};
