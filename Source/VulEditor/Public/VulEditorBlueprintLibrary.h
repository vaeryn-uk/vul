#pragma once

#include "CoreMinimal.h"
#include "DataTable/VulDataRepository.h"
#include "DataTable/VulDataTableSource.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VulEditorBlueprintLibrary.generated.h"

/**
 * Useful functionality exposed to blueprints/python.
 */
UCLASS(BlueprintType)
class VULEDITOR_API UVulEditorBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void ImportConnectedDataSources(UVulDataRepository* Repo);

	static int DoConnectedDataSourceImport(UVulDataRepository* Repo, TArray<FString>& Succeeded, TArray<FString>& Failed);
};
