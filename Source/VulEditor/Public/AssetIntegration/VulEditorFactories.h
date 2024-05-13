#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VulEditorFactories.generated.h"

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

UCLASS()
class UVulDataRepositoryFactory : public UFactory
{
	GENERATED_BODY()

public:
	UVulDataRepositoryFactory();

	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;
};

UCLASS()
class UVulButtonStyleGeneratorFactory : public UFactory
{
	GENERATED_BODY()

public:
	UVulButtonStyleGeneratorFactory();

	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;
};
