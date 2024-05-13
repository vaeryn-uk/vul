#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EditorAssetLibrary.h"
#include "VulRuntime/Public/VulEditorUtil.h"
#include "VulEditorUtility.h"

/**
 * Reused code amongst style generators.
 */
class FVulStyleGeneration
{
public:
	template <typename StyleClass, typename VariantClass>
	static void GenerateStyles(
		StyleClass* Template,
		UObject* Generator,
		const FString& Prefix,
		const TMap<FString, VariantClass>& Variations,
		const TFunction<void (StyleClass*, const VariantClass&)> Apply
	) {
		if (!IsValid(Template))
		{
			FVulEditorUtil::Output(
				INVTEXT("Vul Style Generation"),
				FText::FromString(FString::Printf(TEXT("No style template set"))),
				EAppMsgCategory::Error
			);
			return;
		}

		for (const auto& Variation : Variations)
		{
			const auto Name = FString::Printf(TEXT("%s_%s"), *Prefix, *Variation.Key);
			const auto Directory = FPaths::GetPath(GetPathNameSafe(Generator));
			const auto Path = FString::Printf(TEXT("%s/%s.%s"), *Directory, *Name, *Name);

			UObject* Object;

			if (const auto Exists = UEditorAssetLibrary::DoesAssetExist(Path); Exists)
			{
				Object = UEditorAssetLibrary::LoadAsset(Path);
			} else
			{
				Object = VulEditor::CreateBlueprintAsset<StyleClass>(Name, Directory);
			}

			const auto Blueprint = Cast<UBlueprint>(Object);

			if (!IsValid(Object))
			{
				FVulEditorUtil::Output(
					INVTEXT("Vul Style Generation"),
					FText::FromString(FString::Printf(TEXT("Could not resolve style blueprint object for %s"), *Path)),
					EAppMsgCategory::Error
				);
				return;
			}

			StyleClass* Style = VulEditor::GetBlueprintCDO<StyleClass>(Blueprint);

			if (!IsValid(Style))
			{
				FVulEditorUtil::Output(
					INVTEXT("Vul Style Generation"),
					FText::FromString(FString::Printf(TEXT("No CDO available for style %s"), *Path)),
					EAppMsgCategory::Error
				);
				return;
			}

			UEngine::CopyPropertiesForUnrelatedObjects(Template, Style);

			Apply(Style, Variation.Value);

			Style->MarkPackageDirty();

			UEditorAssetLibrary::SaveAsset(Path, false);
		}
	}
};
