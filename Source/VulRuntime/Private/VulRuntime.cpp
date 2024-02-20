#include "VulRuntime.h"

#define LOCTEXT_NAMESPACE "FVulRuntimeModule"

DEFINE_LOG_CATEGORY(LogVul)

void FVulRuntimeModule::StartupModule()
{
}

void FVulRuntimeModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FVulRuntimeModule, VulRuntime)