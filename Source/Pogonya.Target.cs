using UnrealBuildTool;
public class PogonyaTarget : TargetRules {
    public PogonyaTarget(TargetInfo Target) : base(Target) {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        ExtraModuleNames.Add("Pogonya");
    }
}

