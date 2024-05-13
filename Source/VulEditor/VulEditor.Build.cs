using UnrealBuildTool;

public class VulEditor : ModuleRules
{
    public VulEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "AssetTools", "UnrealYAML", "VulRuntime",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd", "EditorScriptingUtilities",
                "Json", "UnrealYAML", "ToolMenus", "Blutility",
                "CommonUI",
            }
        );
    }
}