using UnrealBuildTool;
using System.Collections.Generic;
public class MoonfallUnrealTarget : TargetRules
{
 public MoonfallUnrealTarget(TargetInfo Target) : base(Target) { Type = TargetType.Game; DefaultBuildSettings = BuildSettingsVersion.V5; ExtraModuleNames.Add("MoonfallUnreal"); }
}
