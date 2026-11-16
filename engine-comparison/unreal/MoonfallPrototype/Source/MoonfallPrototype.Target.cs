using UnrealBuildTool;
using System.Collections.Generic;

public class MoonfallPrototypeTarget : TargetRules
{
    public MoonfallPrototypeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.Add("MoonfallPrototype");
    }
}
