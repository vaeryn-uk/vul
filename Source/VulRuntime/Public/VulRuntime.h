#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVul, Display, Display)

class FVulRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

/**
 * Declares a weak object ptr that we load on-demand and cache. Defines a Resolve<Name> method
 * which returns true if the property is safe to use.
 *
 * Useful for objects that need a specialized pointer to a subclass for its lifetime and accessed
 * commonly, such as in a Tick function.
 */
#define DECLARE_VUL_LAZY_OBJ_PTR(Type, Name) \
mutable TWeakObjectPtr<Type> Name; \
bool Resolve##Name() const; \

/**
 * Implements the resolution of DECLARE_VUL_LAZY_OBJ_PTR. Load should assign to Name.
 */
#define DEFINE_VUL_LAZY_OBJ_PTR(Class, Name, Load) \
bool Class::Resolve##Name() const \
{ \
if (!Name.IsValid()) \
{ \
Load; \
} \
return Name.IsValid(); \
}

/**
 * For one-liner Load. This is assigned to Name.
 *
 * DEFINE_VUL_LAZY_OBJ_PTR_SHORT(MyClass, MyObjPtr, GetComponentByClass<MyType>)
 */
#define DEFINE_VUL_LAZY_OBJ_PTR_SHORT(Class, Name, Load) \
bool Class::Resolve##Name() const \
{ \
if (!Name.IsValid()) \
{ \
Name = Load; \
} \
return Name.IsValid(); \
}

