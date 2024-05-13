#pragma once

#include "CoreMinimal.h"
#include "AssetToolsModule.h"
#include "Factories/BlueprintFactory.h"
#include "UObject/Object.h"

namespace VulEditor
{
	template <typename BlueprintClass>
	UBlueprint* CreateBlueprintAsset(const FString& AssetName, const FString& PackagePath)
	{
		FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = BlueprintClass::StaticClass();
		const auto Object = AssetTools.Get().CreateAsset(AssetName, PackagePath, UBlueprint::StaticClass(), Factory);
		return Cast<UBlueprint>(Object);
	}

	template <typename Class>
	Class* GetBlueprintCDO(UBlueprint* Blueprint)
	{
		return Cast<Class>(Blueprint->GeneratedClass->GetDefaultObject<Class>());
	}
}
