#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FVulRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
