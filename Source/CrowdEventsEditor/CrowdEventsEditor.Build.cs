using UnrealBuildTool;

public class CrowdEventsEditor : ModuleRules
{
	public CrowdEventsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CrowdEvents"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"EditorFramework",
			"InputCore",
			"LevelEditor",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"WorkspaceMenuStructure"
		});
	}
}
