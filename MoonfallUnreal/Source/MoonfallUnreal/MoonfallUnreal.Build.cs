using UnrealBuildTool;
public class MoonfallUnreal : ModuleRules
{
    public MoonfallUnreal(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "Projects" });
    }
}
