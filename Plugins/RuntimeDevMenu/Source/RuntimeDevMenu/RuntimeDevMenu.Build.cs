// Copyright (c) 2026 Andrea. All Rights Reserved.

using UnrealBuildTool;

public class RuntimeDevMenu : ModuleRules
{
	public RuntimeDevMenu(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "Projects" });

		// The menu font is read from disk: stage it into the pak in packaged builds too.
		RuntimeDependencies.Add("$(PluginDir)/Resources/Fonts/JetBrainsMono-Regular.ttf", StagedFileType.UFS);
		RuntimeDependencies.Add("$(PluginDir)/Resources/Fonts/JetBrainsMono-Bold.ttf", StagedFileType.UFS);
	}
}
