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

    private void SetupPlatformDefine(UnrealTargetPlatform TargetPlatform)
    {
        if (TargetPlatform == UnrealTargetPlatform.PS4 || TargetPlatform == UnrealTargetPlatform.PS5)
        {
            PublicDefinitions.Add("OSS_PLAYFAB_PLAYSTATION=1");
        }
        PublicDefinitions.Add(String.Format("OSS_PLAYFAB_{0}=1", TargetPlatform.ToString().ToUpper()));
        System.Console.WriteLine("OnlineSubsystemPlayFab: building for platform {0}", TargetPlatform.ToString().ToUpper());
    }

    public OnlineSubsystemPlayFab(ReadOnlyTargetRules Target) : base(Target)
    {
        // We don't want to try and load when doing project gen, editor, server, etc
        if (Target.bGenerateProjectFiles)
        {
            PrecompileForTargets = PrecompileTargetsType.None;
            return;
        }

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        AddCommonModuleDependencies();
  
        SetCommonDefinitions();

        SetupPlatformDefine(Target.Platform);

        PlayFabOSSSupportedPlatformType Platform = DeterminePlatformType();

        IPlayFabOSSPlatformConfigurator PlatformConfigurator = GetConfiguratorForPlatform(Platform);

        PublicDefinitions.Add("OSS_PLAYFAB_IS_PC=" + (PlatformConfigurator.IsPCPlatform(Target) ? "1" : "0"));

        PlatformConfigurator.AddModuleDependencies(this);
        PlatformConfigurator.SetPlatformDefinitions(this);
        PlatformConfigurator.ConfigurePlayFabDependencies(Target, this);
    }

    private void AddCommonModuleDependencies()
    {
        PublicDependencyModuleNames.AddRange(
        new string[] {
                "OnlineSubsystemUtils",
        });

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
    }

    private void SetCommonDefinitions()
    {
        PublicDefinitions.Add("PLAYFAB_SUBSYSTEM=FName(TEXT(\"PLAYFAB\"))");
        PrivateDefinitions.Add("ONLINESUBSYSTEMPLAYFAB_PACKAGE=1");

        // For debugging purposes
        PublicDefinitions.Add("OSS_PLAYFAB_VERBOSE_VOIP_LOGGING=" + (VerboseVOIPLoggingEnabled ? "1" : "0"));
        PublicDefinitions.Add("OSS_PLAYFAB_VERBOSE_PACKET_LEVEL_LOGGING=" + (VerbosePacketLevelLoggingEnabled ? "1" : "0"));
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

    private enum PlayFabOSSSupportedPlatformType
    {
        Undefined,
        GDK,
        Windows,
        Switch,
        PS4,
        PS5
    }

    private PlayFabOSSSupportedPlatformType DeterminePlatformType()
    {
        // Because GDK platform determination relies on a type that only exists in GDK enabled builds of the UE, we use reflection to dynamically
        // look for the existence of the GDK field on the UnrealPlatformGroup type.
        FieldInfo UnrealPlatformGroupGDKField = typeof(UnrealPlatformGroup).GetField("GDK", BindingFlags.Public | BindingFlags.Static);
        if (UnrealPlatformGroupGDKField != null)
        {
            System.Console.WriteLine("OnlineSubsystemPlayFab: GDK Platform Field Exists!");
            UnrealPlatformGroup GDK = (UnrealPlatformGroup)UnrealPlatformGroupGDKField.GetValue(null);
            if (GDK != null && Target.Platform.IsInGroup(GDK))
            {
                 return PlayFabOSSSupportedPlatformType.GDK;
            }
        }

        if (Target.Platform == UnrealTargetPlatform.Switch)
        {
            return PlayFabOSSSupportedPlatformType.Switch;
        }

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            return PlayFabOSSSupportedPlatformType.Windows;
        }

        if (Target.Platform == UnrealTargetPlatform.PS4)
        {
            return PlayFabOSSSupportedPlatformType.PS4;
        }

        if (Target.Platform == UnrealTargetPlatform.PS5)
        {
            return PlayFabOSSSupportedPlatformType.PS5;
        }
        
        throw new PlatformNotSupportedException(Target.Platform.ToString() + " is not supported.");
    }

    private IPlayFabOSSPlatformConfigurator GetConfiguratorForPlatform(PlayFabOSSSupportedPlatformType Platform)
    {
        switch (Platform)
        {
            case PlayFabOSSSupportedPlatformType.GDK:
                {
                    return new GDKPlatformConfigurator();
                }
            case PlayFabOSSSupportedPlatformType.Switch:
                {
                    return new SwitchPlatformConfigurator();
                }
            case PlayFabOSSSupportedPlatformType.Windows:
                {
                    return new WindowsPlatformConfigurator();
                }
            case PlayFabOSSSupportedPlatformType.PS4:
                {
                    return new PlayStation4PlatformConfigurator();
                }
            case PlayFabOSSSupportedPlatformType.PS5:
                {
                    return new PlayStation5PlatformConfigurator();
                }
            default:
                {
                    throw new PlatformNotSupportedException(Target.Platform.ToString() + " is not supported.");
                }
        }
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

    public interface IPlayFabOSSPlatformConfigurator
    {
        bool IsPCPlatform(ReadOnlyTargetRules Target);
        void AddModuleDependencies(ModuleRules ThisModule);
        void SetPlatformDefinitions(ModuleRules ThisModule);
        void ConfigurePlayFabDependencies(ReadOnlyTargetRules Target, ModuleRules ThisModule);
    }

    private class GDKPlatformConfigurator : IPlayFabOSSPlatformConfigurator
    {
        public bool IsPCPlatform(ReadOnlyTargetRules Target)
        {
            return Target.Platform == UnrealTargetPlatform.Parse("WinGDK");
        }

        public void AddModuleDependencies(ModuleRules ThisModule)
        {
            ThisModule.PublicDependencyModuleNames.Add("GDKCore");
            ThisModule.PublicDependencyModuleNames.Add("OnlineSubsystemGDK");
        }

        public void SetPlatformDefinitions(ModuleRules ThisModule)
        {
            ThisModule.PrivateDefinitions.Add("ONLINESUBSYSTEMGDK_PACKAGE=1");
        }

        public void ConfigurePlayFabDependencies(ReadOnlyTargetRules Target, ModuleRules ThisModule)
        {
            string PlatformDir = Path.Combine(Directory.GetCurrentDirectory(), "..", "Plugins", "Online", "OnlineSubsystemPlayFab", "Platforms", "GDK");

            if (!Directory.Exists(PlatformDir) ||
                !Directory.Exists(Path.Combine(PlatformDir, "Redist")) ||
                !Directory.Exists(Path.Combine(PlatformDir, "Include")) ||
                !Directory.Exists(Path.Combine(PlatformDir, "Lib"))
                )
            {
                throw new BuildException("PlayFab precompiled dependencies were not found for the platform you are trying to compile.");
            }

            string PlayFabPartyIncludePath = Path.Combine(PlatformDir, "Include");
            ThisModule.PublicSystemIncludePaths.Add(PlayFabPartyIncludePath);

            string PlayFabPartyLibraryPath = Path.Combine(PlatformDir, "Lib");
            ThisModule.PublicSystemLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "Party.lib"));
            ThisModule.PublicSystemLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "PartyXboxLive.lib"));
            ThisModule.PublicAdditionalLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "PlayFabMultiplayerGDK.lib"));

            string PlayFabPartyRedistPath = Path.Combine(PlatformDir, "Redist");
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/Party.dll", Path.Combine(PlayFabPartyRedistPath, "Party.dll"), StagedFileType.SystemNonUFS);
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/Party.pdb", Path.Combine(PlayFabPartyRedistPath, "Party.pdb"), StagedFileType.DebugNonUFS);

            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/PartyXboxLive.dll", Path.Combine(PlayFabPartyRedistPath, "PartyXboxLive.dll"), StagedFileType.SystemNonUFS);
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/PartyXboxLive.pdb", Path.Combine(PlayFabPartyRedistPath, "PartyXboxLive.pdb"), StagedFileType.DebugNonUFS);

            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerGDK.dll", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerGDK.dll"), StagedFileType.SystemNonUFS);
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerGDK.pdb", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerGDK.pdb"), StagedFileType.DebugNonUFS);

            string XCurlRedistDir = GDKExports.GetExtensionDirectory("Xbox.XCurl.API", true);
            if (XCurlRedistDir != null)
            {
                ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/XCurl.dll", Path.Combine(XCurlRedistDir, "XCurl.dll"), StagedFileType.SystemNonUFS);
                ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/XCurl.pdb", Path.Combine(XCurlRedistDir, "XCurl.pdb"), StagedFileType.DebugNonUFS);
                ThisModule.PublicDelayLoadDLLs.Add("XCurl.dll");
            }
        }
    }

    private class SwitchPlatformConfigurator : IPlayFabOSSPlatformConfigurator
    {
        public bool IsPCPlatform(ReadOnlyTargetRules Target)
        {
            return false;
        }

        public void AddModuleDependencies(ModuleRules ThisModule)
        {
            ThisModule.PublicDependencyModuleNames.Add("OnlineSubsystemSwitch");
        }

        public void SetPlatformDefinitions(ModuleRules ThisModule)
        {
            //No switch specific platform definitions.
        }

        public void ConfigurePlayFabDependencies(ReadOnlyTargetRules Target, ModuleRules ThisModule)
        {
            string PlatformDir = Path.Combine(Directory.GetCurrentDirectory(), "..", "Plugins", "Online", "OnlineSubsystemPlayFab", "Platforms", "Switch");

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

            ThisModule.PublicSystemIncludePaths.Add(PartyIncludePath);
            ThisModule.PublicAdditionalLibraries.Add(Path.Combine(PartyLibraryPath, "Party.nrs"));
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/Party.nro", Path.Combine(PartyLibraryPath, "Party.nro"), StagedFileType.SystemNonUFS);
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/Party.nrr", Path.Combine(PartyLibraryPath, "Party.nrr"), StagedFileType.DebugNonUFS);

            // Load Multiplayer binaries
            string MultiplayerIncludePath = Path.Combine(PlatformDir, NugetPackageInfo.MultiplayerPackagePath, "build", "native", "include");
            string MultiplayerLibraryPath = Path.Combine(PlatformDir, NugetPackageInfo.MultiplayerPackagePath, "build", "native", "lib", "NX64", "release");

            if (!Directory.Exists(MultiplayerIncludePath) ||
                !Directory.Exists(MultiplayerLibraryPath))
            {
                throw new BuildException("PlayFab Multiplayer precompiled dependencies were not found.");
            }

            ThisModule.PublicSystemIncludePaths.Add(MultiplayerIncludePath);
            ThisModule.PublicAdditionalLibraries.Add(Path.Combine(MultiplayerLibraryPath, "libPlayFabMultiplayer.nrs"));
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/libPlayFabMultiplayer.nro", Path.Combine(MultiplayerLibraryPath, "libPlayFabMultiplayer.nro"), StagedFileType.SystemNonUFS);
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/libPlayFabMultiplayer.nrr", Path.Combine(MultiplayerLibraryPath, "libPlayFabMultiplayer.nrr"), StagedFileType.DebugNonUFS);
            
            // Load Nintendo SDK dependencies.
            string NintendoSdkRoot = Environment.GetEnvironmentVariable("NINTENDO_SDK_ROOT");
            if (String.IsNullOrWhiteSpace(NintendoSdkRoot))
            {
                throw new BuildException("NINTENDO_SDK_ROOT environment was not found for the platform you are trying to compile.");
            }

            string configuration = (Target.Configuration == UnrealTargetConfiguration.Shipping || Target.Configuration == UnrealTargetConfiguration.Test) ? "Release" : "Develop";

            ThisModule.PublicAdditionalLibraries.Add(Path.Combine(NintendoSdkRoot, "Libraries/NX-NXFP2-a64", configuration, "libnn_websocket.a"));
        }
    }

    private class WindowsPlatformConfigurator : IPlayFabOSSPlatformConfigurator
    {
        public bool IsPCPlatform(ReadOnlyTargetRules Target)
        {
            return true;
        }

        public void AddModuleDependencies(ModuleRules ThisModule)
        {
            //No windows specific module dependencies.
        }

        public void SetPlatformDefinitions(ModuleRules ThisModule)
        {
            //No windows specific platform definitions.
        }

        public void ConfigurePlayFabDependencies(ReadOnlyTargetRules Target, ModuleRules ThisModule)
        {
            string PlatformDir = Path.Combine(Directory.GetCurrentDirectory(), "..", "Plugins", "Online", "OnlineSubsystemPlayFab", "Platforms", "Windows");

            if (!Directory.Exists(PlatformDir) || 
                !Directory.Exists(Path.Combine(PlatformDir, "Redist")) || 
                !Directory.Exists(Path.Combine(PlatformDir, "Include")) ||
                !Directory.Exists(Path.Combine(PlatformDir, "Lib"))
                )
            {
                throw new BuildException("PlayFab precompiled dependencies were not found for the platform you are trying to compile.");
            }

            string PlayFabPartyIncludePath = Path.Combine(PlatformDir, "Include");
            ThisModule.PublicSystemIncludePaths.Add(PlayFabPartyIncludePath);

            string PlayFabPartyLibraryPath = Path.Combine(PlatformDir, "Lib");
            ThisModule.PublicSystemLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "Party.lib"));
            ThisModule.PublicAdditionalLibraries.Add(Path.Combine(PlayFabPartyLibraryPath, "PlayFabMultiplayerWin.lib"));

            string PlayFabPartyRedistPath = Path.Combine(PlatformDir, "Redist");
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/PartyWin.dll", Path.Combine(PlayFabPartyRedistPath, "PartyWin.dll"), StagedFileType.SystemNonUFS);
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/PartyWin.pdb", Path.Combine(PlayFabPartyRedistPath, "PartyWin.pdb"), StagedFileType.DebugNonUFS);
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerWin.dll", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerWin.dll"), StagedFileType.SystemNonUFS);
            ThisModule.RuntimeDependencies.Add("$(TargetOutputDir)/PlayFabMultiplayerWin.pdb", Path.Combine(PlayFabPartyRedistPath, "PlayFabMultiplayerWin.pdb"), StagedFileType.DebugNonUFS);
        }
    }

    private class PlayStationPlatformConfigurator : IPlayFabOSSPlatformConfigurator
    {
        protected string PlatformDir;
        protected NuGetPackageLoader NuGetLoader;
        protected NuGetPackageLoader.NuGetPackageInformation NugetPackageInfo;

        public virtual bool IsPCPlatform(ReadOnlyTargetRules Target)
        {
            return false;
        }

        public virtual void AddModuleDependencies(ModuleRules ThisModule)
        {
            //No common playstation module dependencies.
        }

        public virtual void SetPlatformDefinitions(ModuleRules ThisModule)
        {
            ThisModule.bAllowConfidentialPlatformDefines = true;
        }

        public virtual void ConfigurePlayFabDependencies(ReadOnlyTargetRules Target, ModuleRules ThisModule)
        {
            //No common playstation platform definitions.
        }
    }

    private class PlayStation4PlatformConfigurator : PlayStationPlatformConfigurator
    {
        public PlayStation4PlatformConfigurator()
        {
            NugetPackageInfo = new NuGetPackageLoader.NuGetPackageInformation();
            NuGetLoader = new NuGetPackageLoader();
            PlatformDir = Path.Combine(Directory.GetCurrentDirectory(), "..", "Plugins", "Online", "OnlineSubsystemPlayFab", "Platforms", "PS4");
            NuGetLoader.ParsingNuGetPackage(ref PlatformDir, ref NugetPackageInfo);
        }

        public override void AddModuleDependencies(ModuleRules ThisModule)
        {
            ThisModule.PublicDependencyModuleNames.Add("OnlineSubsystemPS4");
        }

        public override void SetPlatformDefinitions(ModuleRules ThisModule)
        {
            base.SetPlatformDefinitions(ThisModule);

            //No PlayStation4 specific platform definitions.
        }

        public override void ConfigurePlayFabDependencies(ReadOnlyTargetRules Target, ModuleRules ThisModule)
        {
        }
    }

    private class PlayStation5PlatformConfigurator : PlayStationPlatformConfigurator
    {
        public PlayStation5PlatformConfigurator()
        {
            NugetPackageInfo = new NuGetPackageLoader.NuGetPackageInformation();
            NuGetLoader = new NuGetPackageLoader();
            PlatformDir = Path.Combine(Directory.GetCurrentDirectory(), "..", "Plugins", "Online", "OnlineSubsystemPlayFab", "Platforms", "PS5");
            NuGetLoader.ParsingNuGetPackage(ref PlatformDir, ref NugetPackageInfo);
        }

        public override void AddModuleDependencies(ModuleRules ThisModule)
        {
            ThisModule.PublicDependencyModuleNames.Add("OnlineSubsystemPS5");
        }

        public override void SetPlatformDefinitions(ModuleRules ThisModule)
        {
            base.SetPlatformDefinitions(ThisModule);

            //No PlayStation5 specific platform definitions.
        }

        public override void ConfigurePlayFabDependencies(ReadOnlyTargetRules Target, ModuleRules ThisModule)
        {
        }
    }
}
