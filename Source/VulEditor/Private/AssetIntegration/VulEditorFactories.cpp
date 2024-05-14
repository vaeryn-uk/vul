#include "AssetIntegration/VulEditorFactories.h"
#include "DataTable/VulDataRepository.h"
#include "DataTable/VulDataTableSource.h"
#include "StyleGenerator/VulBorderStyleGenerator.h"
#include "StyleGenerator/VulButtonStyleGenerator.h"
#include "StyleGenerator/VulTextStyleGenerator.h"

UVulDataTableSourceFactory::UVulDataTableSourceFactory()
{
	SupportedClass = UVulDataTableSource::StaticClass();
	bCreateNew = true;
}

UObject* UVulDataTableSourceFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UVulDataTableSource>(InParent, InClass, InName, Flags, Context);
}

UVulDataRepositoryFactory::UVulDataRepositoryFactory()
{
	SupportedClass = UVulDataRepository::StaticClass();
	bCreateNew = true;
}

UObject* UVulDataRepositoryFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UVulDataRepository>(InParent, InClass, InName, Flags, Context);
}

UVulButtonStyleGeneratorFactory::UVulButtonStyleGeneratorFactory()
{
	SupportedClass = UVulButtonStyleGenerator::StaticClass();
	bCreateNew = true;
}

UObject* UVulButtonStyleGeneratorFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UVulButtonStyleGenerator>(InParent, InClass, InName, Flags, Context);
}

UVulTextStyleGeneratorFactory::UVulTextStyleGeneratorFactory()
{
	SupportedClass = UVulTextStyleGenerator::StaticClass();
	bCreateNew = true;
}

UObject* UVulTextStyleGeneratorFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UVulTextStyleGenerator>(InParent, InClass, InName, Flags, Context);
}

UVulBorderStyleGeneratorFactory::UVulBorderStyleGeneratorFactory()
{
	SupportedClass = UVulBorderStyleGenerator::StaticClass();
	bCreateNew = true;
}

UObject* UVulBorderStyleGeneratorFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UVulBorderStyleGenerator>(InParent, InClass, InName, Flags, Context);
}
