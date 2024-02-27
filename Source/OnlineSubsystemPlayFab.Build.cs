//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

using UnrealBuildTool;
using System;
using System.IO;
using System.Reflection;
using System.Linq;
using System.Diagnostics;
using System.Threading;

public class OnlineSubsystemPlayFab : ModuleRules
{
    private static bool VerboseVOIPLoggingEnabled = false;
    private static bool VerbosePacketLevelLoggingEnabled = false;

    public OnlineSubsystemPlayFab(ReadOnlyTargetRules Target) : base(Target)
    {
        // We don't want to try and load when doing project gen
        if (Target.bGenerateProjectFiles)
        {
            PrecompileForTargets = PrecompileTargetsType.None;
            return;
        }

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Common Public Module Dependencies
		PublicDependencyModuleNames.AddRange(
		new string[] {
				"OnlineSubsystemUtils",
		});

		// Common Private Module Dependencies
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

		// Common Definitions
		PublicDefinitions.Add(String.Format("OSS_PLAYFAB_{0}=1", Target.Platform.ToString().ToUpper()));
		PublicDefinitions.Add("PLAYFAB_SUBSYSTEM=FName(TEXT(\"PLAYFAB\"))");
		PrivateDefinitions.Add("ONLINESUBSYSTEMPLAYFAB_PACKAGE=1");

		// For debugging purposes
		PublicDefinitions.Add("OSS_PLAYFAB_VERBOSE_VOIP_LOGGING=" + (VerboseVOIPLoggingEnabled ? "1" : "0"));
		PublicDefinitions.Add("OSS_PLAYFAB_VERBOSE_PACKET_LEVEL_LOGGING=" + (VerbosePacketLevelLoggingEnabled ? "1" : "0"));

		ConfigureForPlatform();
	}

    // Because GDK platform determination relies on a type, GDKExports" that only exists in GDK enabled builds of the UE, we use reflection to dynamically
    // look for the existence of the GDKExports type and expose the needed methods here via MethodInfo.
    private static class GDKExports
    {
        private static Type ReflectedType;
        private static MethodInfo GetExtendedDirectoryMethodInfo = null;

        static GDKExports()
        {
            ReflectedType = FindType();
            if (ReflectedType == null)
            {
                throw new Exception("Failed to locate GDKExports");
            }
            else
            {
                GetExtendedDirectoryMethodInfo = ReflectedType.GetMethod("GetExtensionDirectory", BindingFlags.Static | BindingFlags.Public | BindingFlags.FlattenHierarchy );
                if (GetExtendedDirectoryMethodInfo == null)
                {
                    throw new Exception("Faild to locate GetExtensionDirectory");
                }
            }
        }

        private static Type FindType()
        {
            return AppDomain.CurrentDomain.GetAssemblies()
                    .FirstOrDefault(x => x.GetName().Name.Equals("UnrealBuildTool"))
                    .GetTypes()
                    .FirstOrDefault(x => x.Name.Equals("GDKExports"));
        }

        public static string GetExtensionDirectory(string ExtensionDirectory, bool IsForRedistribution)
        {
            return GetExtendedDirectoryMethodInfo != null ? (string)GetExtendedDirectoryMethodInfo.Invoke(null, new object[] { ExtensionDirectory, IsForRedistribution }) : string.Empty;
        }
    }

	private void ConfigureForPlatform()
	{
		// Because platform determination relies on types that only exist in builds that have the platform enabled, we use reflection to dynamically 
		// look for the existence of the platform's field on the UnrealPlatformGroup type.

		//GDK
		FieldInfo Field = typeof(UnrealPlatformGroup).GetField("GDK", BindingFlags.Public | BindingFlags.Static);
		if (Field != null)
		{
			UnrealPlatformGroup Group = (UnrealPlatformGroup)Field.GetValue(null);
			if (Target.Platform.IsInGroup(Group))
			{
				ConfigureForGDKPlatform();
				return;
			}
		}

        //Switch
        if (Target.Platform.ToString() == "Switch")
        {
            ConfigureForSwitchPlatform();
            return;

        }

        //Win64
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            ConfigureForWindowsPlatform();
            return;
        }

        //PS4
        if (Target.Platform.ToString() == "PS4")
        {
            ConfigureForPlayStation4Platform();
            return;
        }

        //PS5
        if (Target.Platform.ToString() == "PS5")
        {
            ConfigureForPlayStation5Platform();
            return;
        }

        throw new PlatformNotSupportedException(Target.Platform.ToString() + " is not supported.");
	}

    private class NuGetPackageLoader
    {
        public class NuGetPackageInformation
        {
            public string PartyPackagePath = string.Empty;
            public string MultiplayerPackagePath = string.Empty;
        }

        // If want specific version to use, can specify the version in the packages.config file.
        public void ParsingNuGetPackage(ref string PlatformDir, ref NuGetPackageInformation PackageInfo)
        {
            string[] Lines = System.IO.File.ReadAllLines(Path.Combine(PlatformDir, "packages.config"));
            foreach (string Line in Lines)
            {
                Int32 BeginOfString = Line.IndexOf("Microsoft.PlayFab", 0);
                if (BeginOfString > -1)
                {
                    Int32 EndOfString = Line.IndexOf("\"", BeginOfString);
                    string Id = Line.Substring(BeginOfString, EndOfString - BeginOfString);

                    const string versionString = "version=\"";
                    BeginOfString = Line.IndexOf(versionString, 0);
                    if (BeginOfString > -1)
                    {
                        BeginOfString += versionString.Length;
                        EndOfString = Line.IndexOf("\"", BeginOfString);
                        string Version = Line.Substring(BeginOfString, EndOfString - BeginOfString);

                        if (Id.IndexOf("Party", 0) > -1)
                        {
                            PackageInfo.PartyPackagePath = Id + "." + Version;
                        }
                        else if (Id.IndexOf("Multiplayer", 0) > -1)
                        {
                            PackageInfo.MultiplayerPackagePath = Id + "." + Version;
                        }
                        else
                        {
                            throw new BuildException("Unknown package id in package.config file.");
                        }
                    }
                }
            }
            if (PackageInfo.PartyPackagePath.Length == 0 && PackageInfo.MultiplayerPackagePath.Length == 0)
            {
                throw new BuildException("Can't find Party and Multiplayer package infomation in package.config file.");
            }
            System.Console.WriteLine("OnlineSubsystemPlayFab: Party={0}", PackageInfo.PartyPackagePath);
            System.Console.WriteLine("OnlineSubsystemPlayFab: Multiplayer={0}", PackageInfo.MultiplayerPackagePath);
        }
    }

	//GDK
    private void ConfigureForGDKPlatform()
	{
        PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("OnlineSubsystemGDK");

		PublicDefinitions.Add("OSS_PLAYFAB_GDK=1");

        if(Target.Platform == UnrealTargetPlatform.Parse("WinGDK"))
		{
			PublicDefinitions.Add("OSS_PLAYFAB_IS_PC=1");
		}

		PrivateDefinitions.Add("ONLINESUBSYSTEMGDK_PACKAGE=1");

        string PlatformDir = Path.Combine(PluginDirectory, "Platforms", "GDK");

        if (!Directory.Exists(PlatformDir) ||
            !Directory.Exists(Path.Combine(PlatformDir, "Redist")) ||
            !Directory.Exists(Path.Combine(PlatformDir, "Include")) ||
            !Directory.Exists(Path.Combine(PlatformDir, "Lib"))
            )
        {
            throw new BuildException("PlayFab precompiled dependencies were not found for the platform you are trying to compile.");
        }

        string PlayFabPartyIncludePath = Path.Combine(PlatformDir, "Include");
        PublicSystemIncludePaths.Add(PlayFabPartyIncludePath);

        string PlayFabPartyLibraryPath = Path.Combine(PlatformDir, "Lib");
        PublicSystemLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "Party.lib"));
        PublicSystemLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "PartyXboxLive.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "PlayFabMultiplayerGDK.lib"));

        string PlayFabPartyRedistPath = Path.Combine(PlatformDir, "Redist");
        RuntimeDependencies.Add("$(TargetOutputDir)/Party.dll", Path.Combine(PlayFabPartyRedistPath, "Party.dll"), StagedFileType.SystemNonUFS);
        RuntimeDependencies.Add("$(TargetOutputDir)/Party.pdb", Path.Combine(PlayFabPartyRedistPath, "Party.pdb"), StagedFileType.DebugNonUFS);

        RuntimeDependencies.Add("$(TargetOutputDir)/PartyXboxLive.dll", Path.Combine(PlayFabPartyRedistPath, "PartyXboxLive.dll"), StagedFileType.SystemNonUFS);
        RuntimeDependencies.Add("$(TargetOutputDir)/PartyXboxLive.pdb", Path.Combine(PlayFabPartyRedistPath, "PartyXboxLive.pdb"), StagedFileType.DebugNonUFS);

        RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerGDK.dll", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerGDK.dll"), StagedFileType.SystemNonUFS);
        RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerGDK.pdb", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerGDK.pdb"), StagedFileType.DebugNonUFS);

        string XCurlRedistDir = GDKExports.GetExtensionDirectory("Xbox.XCurl.API", true);
        if (XCurlRedistDir != null)
        {
            RuntimeDependencies.Add("$(TargetOutputDir)/XCurl.dll", Path.Combine(XCurlRedistDir, "XCurl.dll"), StagedFileType.SystemNonUFS);
            RuntimeDependencies.Add("$(TargetOutputDir)/XCurl.pdb", Path.Combine(XCurlRedistDir, "XCurl.pdb"), StagedFileType.DebugNonUFS);
            PublicDelayLoadDLLs.Add("XCurl.dll");
        }
    }

	//Switch
    private void ConfigureForSwitchPlatform()
	{
		PublicDependencyModuleNames.Add("OnlineSubsystemSwitch");

        string PlatformDir = Path.Combine(PluginDirectory, "Platforms", "Switch");

        if (!Directory.Exists(PlatformDir))
        {
            throw new BuildException("PlayFab precompiled dependencies were not found.");
        }

        // Find the MLP and Party library names under the PlatformDir.
        NuGetPackageLoader NuGetLoader = new NuGetPackageLoader();
        NuGetPackageLoader.NuGetPackageInformation NugetPackageInfo = new NuGetPackageLoader.NuGetPackageInformation();
        NuGetLoader.ParsingNuGetPackage(ref PlatformDir, ref NugetPackageInfo);

        // Load Party binaries.
        string PartyIncludePath = Path.Combine(PlatformDir, NugetPackageInfo.PartyPackagePath, "build", "native", "include");
        string PartyLibraryPath = Path.Combine(PlatformDir, NugetPackageInfo.PartyPackagePath, "build", "native", "lib", "NX64", "release");

        if (!Directory.Exists(PartyIncludePath) ||
            !Directory.Exists(PartyLibraryPath))
        {
            throw new BuildException("PlayFab Party precompiled dependencies were not found.");
        }

        PublicSystemIncludePaths.Add(PartyIncludePath);
        PublicAdditionalLibraries.Add(Path.Combine(PartyLibraryPath, "Party.nrs"));
        RuntimeDependencies.Add("$(TargetOutputDir)/Party.nro", Path.Combine(PartyLibraryPath, "Party.nro"), StagedFileType.SystemNonUFS);
        RuntimeDependencies.Add("$(TargetOutputDir)/Party.nrr", Path.Combine(PartyLibraryPath, "Party.nrr"), StagedFileType.DebugNonUFS);

        // Load Multiplayer binaries
        string MultiplayerIncludePath = Path.Combine(PlatformDir, NugetPackageInfo.MultiplayerPackagePath, "build", "native", "include");
        string MultiplayerLibraryPath = Path.Combine(PlatformDir, NugetPackageInfo.MultiplayerPackagePath, "build", "native", "lib", "NX64", "release");

        if (!Directory.Exists(MultiplayerIncludePath) ||
            !Directory.Exists(MultiplayerLibraryPath))
        {
            throw new BuildException("PlayFab Multiplayer precompiled dependencies were not found.");
        }

        PublicSystemIncludePaths.Add(MultiplayerIncludePath);
        PublicAdditionalLibraries.Add(Path.Combine(MultiplayerLibraryPath, "libPlayFabMultiplayer.nrs"));
        RuntimeDependencies.Add("$(TargetOutputDir)/libPlayFabMultiplayer.nro", Path.Combine(MultiplayerLibraryPath, "libPlayFabMultiplayer.nro"), StagedFileType.SystemNonUFS);
        RuntimeDependencies.Add("$(TargetOutputDir)/libPlayFabMultiplayer.nrr", Path.Combine(MultiplayerLibraryPath, "libPlayFabMultiplayer.nrr"), StagedFileType.DebugNonUFS);
            
        // Load Nintendo SDK dependencies.
        string NintendoSdkRoot = Environment.GetEnvironmentVariable("NINTENDO_SDK_ROOT");
        if (String.IsNullOrWhiteSpace(NintendoSdkRoot))
        {
            throw new BuildException("NINTENDO_SDK_ROOT environment was not found for the platform you are trying to compile.");
        }

        string configuration = (Target.Configuration == UnrealTargetConfiguration.Shipping || Target.Configuration == UnrealTargetConfiguration.Test) ? "Release" : "Develop";

        PublicAdditionalLibraries.Add(Path.Combine(NintendoSdkRoot, "Libraries/NX-NXFP2-a64", configuration, "libnn_websocket.a"));
    }

	//Windows
    private void ConfigureForWindowsPlatform()
    {
		PublicDefinitions.Add("OSS_PLAYFAB_IS_PC=1");
		PublicDefinitions.Add("USES_NATIVE_SESSION=1");

		string PlatformDir = Path.Combine(PluginDirectory, "Platforms", "Windows");

        if (!Directory.Exists(PlatformDir) || 
            !Directory.Exists(Path.Combine(PlatformDir, "Redist")) || 
            !Directory.Exists(Path.Combine(PlatformDir, "Include")) ||
            !Directory.Exists(Path.Combine(PlatformDir, "Lib"))
            )
        {
            throw new BuildException("PlayFab precompiled dependencies were not found for the platform you are trying to compile.");
        }

        string PlayFabPartyIncludePath = Path.Combine(PlatformDir, "Include");
        PublicSystemIncludePaths.Add(PlayFabPartyIncludePath);

        string PlayFabPartyLibraryPath = Path.Combine(PlatformDir, "Lib");
        PublicSystemLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "PartyWin.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "PlayFabMultiplayerWin.lib"));

        string PlayFabPartyRedistPath = Path.Combine(PlatformDir, "Redist");
        RuntimeDependencies.Add("$(TargetOutputDir)/PartyWin.dll", Path.Combine(PlayFabPartyRedistPath, "PartyWin.dll"), StagedFileType.SystemNonUFS);
        RuntimeDependencies.Add("$(TargetOutputDir)/PartyWin.pdb", Path.Combine(PlayFabPartyRedistPath, "PartyWin.pdb"), StagedFileType.DebugNonUFS);
        RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerWin.dll", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerWin.dll"), StagedFileType.SystemNonUFS);
        RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerWin.pdb", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerWin.pdb"), StagedFileType.DebugNonUFS);
    }

	//PS4
	private void ConfigureForPlayStation4Platform()
	{
		NuGetPackageLoader.NuGetPackageInformation NugetPackageInfo = new NuGetPackageLoader.NuGetPackageInformation();
		NuGetPackageLoader NuGetLoader = new NuGetPackageLoader();
		string PlatformDir = Path.Combine(PluginDirectory, "Platforms", "PS4");
		NuGetLoader.ParsingNuGetPackage(ref PlatformDir, ref NugetPackageInfo);

		PublicDependencyModuleNames.Add("OnlineSubsystemPS4");

		bAllowConfidentialPlatformDefines = true;
		PublicDefinitions.Add("OSS_PLAYFAB_PLAYSTATION=1");
		PublicDefinitions.Add("USES_NATIVE_SESSION=1");
	}

	//PS5
    private void ConfigureForPlayStation5Platform()
    {
		NuGetPackageLoader.NuGetPackageInformation NugetPackageInfo = new NuGetPackageLoader.NuGetPackageInformation();
		NuGetPackageLoader NuGetLoader = new NuGetPackageLoader();
        string PlatformDir = Path.Combine(PluginDirectory, "Platforms", "PS5");
        NuGetLoader.ParsingNuGetPackage(ref PlatformDir, ref NugetPackageInfo);
    
        PublicDependencyModuleNames.Add("OnlineSubsystemPS5");
		
		bAllowConfidentialPlatformDefines = true;
		PublicDefinitions.Add("OSS_PLAYFAB_PLAYSTATION=1");
		PublicDefinitions.Add("USES_NATIVE_SESSION=1");
	}
}
