using UnrealBuildTool;

public class VulEditor : ModuleRules
{
    public VulEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "AssetTools",
            }
        );

        // IncludeUnrealYaml();

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd", "EditorScriptingUtilities",
                "VulRuntime", "Json", "UnrealYAML"
            }
        );
    }
}