using UnrealBuildTool;
public class PogonyaGraphEditor : ModuleRules {
    public PogonyaGraphEditor(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "../Pogonya"));
        PrivateDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "Pogonya", "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler", "AssetTools", "InputCore", "AIModule", "NavigationSystem" });
    }
}
