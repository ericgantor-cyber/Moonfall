using UnrealBuildTool;
public class MoonfallUnreal : ModuleRules
{
    public MoonfallUnreal(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.Add("Niagara");
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "Projects", "UMG", "Slate", "SlateCore" });
    }
}
