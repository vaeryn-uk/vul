using UnrealBuildTool;

public class VulRuntime : ModuleRules
{
    public VulRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // For casting of native CPP types.
        // Avoids [C4541] 'dynamic_cast' used on polymorphic type 'XXX' with /GR-; unpredictable behavior may result
        // when casting to non-UObjects.
        bUseRTTI = true;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CommonUI", "Niagara", "LevelSequence", "MovieScene", "MovieSceneTracks"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "VulTest",
                "DeveloperSettings",
                "UMG",
                "Json",
            }
        );

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("EditorScriptingUtilities");
        }
    }
}