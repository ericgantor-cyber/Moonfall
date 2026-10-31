using UnrealBuildTool;
using System.Collections.Generic;
public class MoonfallUnrealEditorTarget : TargetRules
{
 public MoonfallUnrealEditorTarget(TargetInfo Target) : base(Target) { Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.V5; ExtraModuleNames.Add("MoonfallUnreal"); }
}
