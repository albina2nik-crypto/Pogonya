using UnrealBuildTool;
public class Pogonya : ModuleRules {
    public Pogonya(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "InputCore", "NetCore", "AIModule" });
    }
}
