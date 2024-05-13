#include "StyleGenerator/VulButtonStyleGenerator.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "VulEditorUtil.h"
#include "Factories/BlueprintFactory.h"
#include "StyleGenerator/VulButtonStyle.h"

void UVulButtonStyleGenerator::Generate()
{
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
		const auto Name = FString::Printf(TEXT("ButtonStyle_%s"), *Variation.Key);
		const auto Path = FString::Printf(TEXT("%s/%s.%s"), *Directory(), *Name, *Name);

		UObject* Object;

		if (const auto Exists = UEditorAssetLibrary::DoesAssetExist(Path); Exists)
		{
			Object = UEditorAssetLibrary::LoadAsset(Path);
		} else
		{
			FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

			UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
			Factory->ParentClass = UVulButtonStyle::StaticClass();
			Object = AssetTools.Get().CreateAsset(Name, *Directory(), UBlueprint::StaticClass(), Factory);
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

		TObjectPtr<UObject> CDO = Blueprint->GeneratedClass->GetDefaultObject<UVulButtonStyle>();
		UVulButtonStyle* Styles = Cast<UVulButtonStyle>(CDO.Get());
		if (!IsValid(Styles))
		{
			FVulEditorUtil::Output(
				INVTEXT("Vul Style Generation"),
				FText::FromString(FString::Printf(TEXT("No CDO available for style %s"), *Path)),
				EAppMsgCategory::Error
			);
			return;
		}

		UEngine::CopyPropertiesForUnrelatedObjects(Template, Styles);

		Styles->NormalBase.SetResourceObject(Variation.Value.NormalBackground);
		Styles->NormalHovered.SetResourceObject(Variation.Value.HoveredBackground);
		Styles->NormalPressed.SetResourceObject(Variation.Value.PressedBackground);

		Styles->MarkPackageDirty();

		UEditorAssetLibrary::SaveAsset(Path, false);
	}
}

FString UVulButtonStyleGenerator::Directory()
{
	return FPaths::GetPath(GetPathNameSafe(this));
}
