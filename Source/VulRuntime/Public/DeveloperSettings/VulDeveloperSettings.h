#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "VulDeveloperSettings.generated.h"

/**
 * Defines a property whose configured value is only respected if UDeveloperSettings bDevMode
 * is enabled. When devmode is disabled, the default value specified here is used.
 *
 * Use this macro underneath a UPROPERTY declaration of a settings property.
 */
#if UE_BUILD_SHIPPING
	#define VUL_DEV_MODE_SETTING(Type, Property, Default) \
	Type Get##Property() const \
		return Default; \
	}
#else
	#define VUL_DEV_MODE_SETTING(Type, Property, Default) \
	Type Get##Property() const \
	{ \
\
		if (bDevMode) { \
			return Property; \
		} \
\
		return Default; \
	}
#endif

/**
 * Development settings that offer a dev mode feature, where settings can be tweaked to
 * easily jumpstart in to a particular game state/level. These settings are ignored when
 * dev mode is disabled, or we're in a shipping build.
 */
UCLASS(Config=Game, DefaultConfig, Abstract)
class VULRUNTIME_API UVulDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * If selected, all other dev-mode-enabled properties will use their configured values.
	 * Otherwise, all settings will use a default value.
	 *
	 * This is useful for development workflows. Specify VUL_DEV_MODE_SETTING properties
	 * in your config, then use the accessor to get it, respecting the project build & this setting.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite)
	bool bDevMode;
};
