//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

using UnrealBuildTool;
using System;
using System.IO;
using Tools.DotNETCommon;

public class OnlineSubsystemPlayFab : ModuleRules
{
	private void SetupPlatformDefine(UnrealTargetPlatform TargetPlatform)
	{
		PublicDefinitions.Add(String.Format("OSS_PLAYFAB_{0}=1", TargetPlatform.ToString().ToUpper()));
		Log.TraceInformation("OnlineSubsystemPlayFab: building for platform {0}", TargetPlatform.ToString().ToUpper());
	}

	public OnlineSubsystemPlayFab(ReadOnlyTargetRules Target) : base(Target)
	{
		// We don't want to try and load when doing project gen, editor, server, etc
		if (Target.bGenerateProjectFiles || (Target.Type != TargetType.Game && Target.Type != TargetType.Client))
		{
			return;
		}

		string PlayFabPartyExtensionPath = string.Empty;
		string PlayFabPartyRedistPath = string.Empty;
		string PlayFabPartyXblExtensionPath = string.Empty;
		string PlayFabPartyXblRedistPath = string.Empty;
		bool bPlatformHasPlayFabPartyBundled = false;

		SetupPlatformDefine(Target.Platform);

		// Use the Platforms folder libs if available, these are not included by default so should be considered an override if present.
		string PlatformDirName = "UNKNOWN";
		if (Target.Platform.IsInGroup(UnrealPlatformGroup.GDK))
		{
			PlatformDirName = "GDK";
			bPlatformHasPlayFabPartyBundled = true;
		}
		else if (Target.Platform == UnrealTargetPlatform.XboxOne)
		{
			PlatformDirName = "XDK";
		}

		string PlatformDir = Path.Combine(Directory.GetCurrentDirectory(), "..", "Plugins", "Online", "OnlineSubsystemPlayFab", "Platforms", PlatformDirName);

		if (Target.Platform == UnrealBuildTool.UnrealTargetPlatform.WinGDK)
		{
			PublicDefinitions.Add("OSS_PLAYFAB_IS_PC=1");
		}
		else
		{
			PublicDefinitions.Add("OSS_PLAYFAB_IS_PC=0");
		}

		// For debugging purposes
		PublicDefinitions.Add("OSS_PLAYFAB_VERBOSE_VOIP_LOGGING=0");
		PublicDefinitions.Add("OSS_PLAYFAB_VERBOSE_PACKET_LEVEL_LOGGING=0");

		// Add OSS definitions
		PublicDefinitions.Add("PLAYFAB_SUBSYSTEM=FName(TEXT(\"PLAYFAB\"))");

		if (Directory.Exists(PlatformDir) && Directory.Exists(Path.Combine(PlatformDir, "Lib")) && Directory.Exists(Path.Combine(PlatformDir, "Redist")) && Directory.Exists(Path.Combine(PlatformDir, "Include")))
		{
			PlayFabPartyXblExtensionPath = PlayFabPartyExtensionPath = PlatformDir;
			PlayFabPartyXblRedistPath = PlayFabPartyRedistPath = Path.Combine(PlatformDir, "Redist");

			Log.TraceInformation("OnlineSubsystemPlayFab: Using PlayFab Party library provided as an override.");

		}
		else if (bPlatformHasPlayFabPartyBundled) // Use GDK version
		{
			PlayFabPartyExtensionPath = GDKExports.GetExtensionDirectory("PlayFab.Party.Cpp", false);
			PlayFabPartyRedistPath = GDKExports.GetExtensionDirectory("PlayFab.Party.Cpp", true);

			if (!Directory.Exists(PlayFabPartyExtensionPath) || !Directory.Exists(PlayFabPartyRedistPath))
			{
				throw new BuildException("PlayFab Party was not found. Please validate your GDK installation or provide an override library.");
			}

			Log.TraceInformation("OnlineSubsystemPlayFab: Using PlayFab Party library from GDK installation at {0}", PlayFabPartyExtensionPath);

			PlayFabPartyXblExtensionPath = GDKExports.GetExtensionDirectory("PlayFab.PartyXboxLive.Cpp", false);
			PlayFabPartyXblRedistPath = GDKExports.GetExtensionDirectory("PlayFab.PartyXboxLive.Cpp", true);

			if (!Directory.Exists(PlayFabPartyXblExtensionPath) || !Directory.Exists(PlayFabPartyXblRedistPath))
			{
				throw new BuildException("PlayFab Party XboxLive Extension was not found. Please validate your GDK installation or provide an override library.");
			}

			Log.TraceInformation("OnlineSubsystemPlayFab: Using PlayFab Party XboxLive library from GDK installation at {0}", PlayFabPartyXblExtensionPath);
		}
		else
		{
			throw new BuildException("PlayFab Party was not found. This platforms SDK does not have PlayFab Party bundled with it, and you have not provided an override library.");
		}

		string PlayFabPartyIncludePath = Path.Combine(PlayFabPartyExtensionPath, "Include");
		string PlayFabPartyLibraryPath = Path.Combine(PlayFabPartyExtensionPath, "Lib");
		string PlayFabPartyXblIncludePath = Path.Combine(PlayFabPartyXblExtensionPath, "Include");
		string PlayFabPartyXblLibraryPath = Path.Combine(PlayFabPartyXblExtensionPath, "Lib");

		PrivateDefinitions.Add("ONLINESUBSYSTEMPLAYFAB_PACKAGE=1");
		PrivateDefinitions.Add("ONLINESUBSYSTEMGDK_PACKAGE=1");
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicSystemIncludePaths.Add(PlayFabPartyIncludePath);
		PublicSystemIncludePaths.Add(PlayFabPartyXblIncludePath);

		// We need party.lib, this is either in the GDK, or via Platforms override
		PublicSystemLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "Party.lib"));
		PublicSystemLibraries.Add(Path.Combine(PlayFabPartyXblLibraryPath, "PartyXboxLive.lib"));
		PublicAdditionalLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "PlayFabMultiplayerGDK.lib"));
		Log.TraceInformation("OnlineSubsystemPlayFab: Using PlayFab Multiplayer library from GDK installation at {0}", Path.Combine(PlayFabPartyLibraryPath, "PlayFabMultiplayerGDK.lib"));

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"OnlineSubsystemUtils"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"NetCore",
				"Engine",
				"Sockets",
				"Voice",
				"AudioMixer",
				"OnlineSubsystem",
				"Json",
				"PacketHandler",
				"Projects",
				"HTTP"
			}
		);

		// GDK is everything gamecore.
		if (Target.Platform.IsInGroup(UnrealPlatformGroup.GDK))
		{
			PublicDependencyModuleNames.Add("GDKCore");
			PublicDependencyModuleNames.Add("OnlineSubsystemGDK");
		}
		else if (Target.Platform == UnrealTargetPlatform.XboxOne)
		{
			PublicDependencyModuleNames.Add("OnlineSubsystemLive");
		}

		RuntimeDependencies.Add("$(TargetOutputDir)/Party.dll", Path.Combine(PlayFabPartyRedistPath, "Party.dll"), StagedFileType.SystemNonUFS);
		RuntimeDependencies.Add("$(TargetOutputDir)/Party.pdb", Path.Combine(PlayFabPartyRedistPath, "Party.pdb"), StagedFileType.DebugNonUFS);	

		RuntimeDependencies.Add("$(TargetOutputDir)/PartyXboxLive.dll", Path.Combine(PlayFabPartyXblRedistPath, "PartyXboxLive.dll"), StagedFileType.SystemNonUFS);
		RuntimeDependencies.Add("$(TargetOutputDir)/PartyXboxLive.pdb", Path.Combine(PlayFabPartyXblRedistPath, "PartyXboxLive.pdb"), StagedFileType.DebugNonUFS);

		RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerGDK.dll", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerGDK.dll"), StagedFileType.SystemNonUFS);
		RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerGDK.pdb", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerGDK.pdb"), StagedFileType.DebugNonUFS);

		if (Target.Platform.IsInGroup(UnrealPlatformGroup.GDK))
		{
			// Add XCurl redistributable
			string XCurlRedistDir = GDKExports.GetExtensionDirectory("Xbox.XCurl.API", true);
			if (XCurlRedistDir != null)
			{
				RuntimeDependencies.Add("$(TargetOutputDir)/XCurl.dll", Path.Combine(XCurlRedistDir, "XCurl.dll"), StagedFileType.SystemNonUFS);
				RuntimeDependencies.Add("$(TargetOutputDir)/XCurl.pdb", Path.Combine(XCurlRedistDir, "XCurl.pdb"), StagedFileType.DebugNonUFS);
				PublicDelayLoadDLLs.Add("XCurl.dll");
			}
		}
	}
}
