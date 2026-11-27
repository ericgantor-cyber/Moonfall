using UnrealBuildTool;
public class MoonfallUnreal : ModuleRules
{
    public MoonfallUnreal(ReadOnlyTargetRules Target) : base(Target)
    {
        // Never enable V19 in a packaged Game or Shipping target.
        bool Preview = Target.Type == TargetType.Editor &&
            System.Environment.GetEnvironmentVariable("MOONFALL_V19_EDITOR_PREVIEW") == "1";
        PublicDefinitions.Add("MOONFALL_V19_EDITOR_PREVIEW=" + (Preview ? "1" : "0"));
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.Add("Niagara");
        PrivateDependencyModuleNames.Add("RHI");
        PrivateDependencyModuleNames.Add("RenderCore");
        PrivateDependencyModuleNames.Add("Json");
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "Projects", "UMG", "Slate", "SlateCore" });
    }
}
