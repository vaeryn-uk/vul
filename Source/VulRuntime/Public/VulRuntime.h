#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

VULRUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogVul, Display, All)

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

#if defined(__INTELLISENSE__) || defined(__JETBRAINS_IDE__)
	#ifndef USE_RTTI
		#define USE_RTTI 1  // In reality, this'll vary based on build platform, but useful for IDE to show Windows code.
	#endif
#endif

#define VUL_CONCAT_IMPL(x, y) x##y
#define VUL_CONCAT(x, y) VUL_CONCAT_IMPL(x, y)

// TODO: This actual runs multiple times. Once per translation unit.
#define VUL_RUN_ONCE(code_block) \
     VUL_RUN_ONCE_INTERNAL(VUL_CONCAT(FVUL_InitOnceStruct_, __COUNTER__), code_block)

#define VUL_RUN_ONCE_INTERNAL(name, code_block) \
	struct name {                                   \
		name() { code_block; }                      \
	};                                              \
	inline static name VUL_CONCAT(VUL_run_once_instance_, name);
