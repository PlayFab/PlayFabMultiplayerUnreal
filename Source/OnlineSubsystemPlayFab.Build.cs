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
using Tools.DotNETCommon;

public class OnlineSubsystemPlayFab : ModuleRules
{
    private static bool VerboseVOIPLoggingEnabled = false;
    private static bool VerbosePacketLevelLoggingEnabled = false;

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
        Windows
    }

    private PlayFabOSSSupportedPlatformType DeterminePlatformType()
    {
        // Because GDK platform determination relies on a type that only exists in GDK enabled builds of the UE, we use reflection to dynamically
        // look for the existence of the GDK field on the UnrealPlatformGroup type.
        FieldInfo UnrealPlatformGroupGDKField = typeof(UnrealPlatformGroup).GetField("GDK", BindingFlags.Public | BindingFlags.Static);
        if (UnrealPlatformGroupGDKField != null)
        {
            Log.TraceInformation("OnlineSubsystemPlayFab: GDK Platform Field Exists!");
            UnrealPlatformGroup GDK = (UnrealPlatformGroup)UnrealPlatformGroupGDKField.GetValue(null);
            if (GDK != null && Target.Platform.IsInGroup(GDK))
            {
                 return PlayFabOSSSupportedPlatformType.GDK;
            }
        }

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            return PlayFabOSSSupportedPlatformType.Windows;
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
            case PlayFabOSSSupportedPlatformType.Windows:
                {
                    return new WindowsPlatformConfigurator();
                }
            default:
                {
                    throw new PlatformNotSupportedException(Target.Platform.ToString() + " is not supported.");
                }
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
}
