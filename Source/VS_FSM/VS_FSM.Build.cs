// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VS_FSM : ModuleRules
{
	public VS_FSM(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"VS_FSM",
			"VS_FSM/Variant_Platforming",
			"VS_FSM/Variant_Platforming/Animation",
			"VS_FSM/Variant_Combat",
			"VS_FSM/Variant_Combat/AI",
			"VS_FSM/Variant_Combat/Animation",
			"VS_FSM/Variant_Combat/Gameplay",
			"VS_FSM/Variant_Combat/Interfaces",
			"VS_FSM/Variant_Combat/UI",
			"VS_FSM/Variant_SideScrolling",
			"VS_FSM/Variant_SideScrolling/AI",
			"VS_FSM/Variant_SideScrolling/Gameplay",
			"VS_FSM/Variant_SideScrolling/Interfaces",
			"VS_FSM/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
