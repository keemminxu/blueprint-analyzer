// Copyright (c) 2025 keemminxu. All Rights Reserved.

using UnrealBuildTool;

public class BlueprintAnalyzer : ModuleRules
{
	public BlueprintAnalyzer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"BlueprintGraph",
				"KismetCompiler",
				"EditorStyle",
				"EditorWidgets",
				"Json",
				"Kismet",
				"UMG",
				"UMGEditor"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"ToolMenus",
				"EditorSubsystem",
				"AssetTools",
				"ContentBrowser",
				"GraphEditor",
				"DesktopPlatform"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}