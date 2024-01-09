#include "AssetIntegration/VulEditorFactories.h"

#include "DataTable/VulDataRepository.h"
#include "DataTable/VulDataTableSource.h"

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
