using UnrealBuildTool;
public class PogonyaEditorTarget : TargetRules {
    public PogonyaEditorTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        ExtraModuleNames.Add("Pogonya");
    }
}

