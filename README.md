---
title: PlayFab Online Subsystem (OSS) Quickstart
description: Guidance on how to use PlayFab Multiplayer in your Unreal Engine 4 and Unreal Engine 5 Project.
author: Pushpadant Kacha
ms.author: pkacha
ms.date: 04/4/2022
ms.topic: article
ms.prod: playfab
keywords: playfab, multiplayer, networking, unreal, unreal engine, unreal engine 4, unreal engine 5, ue4, ue5, middleware
---

# PlayFab Multiplayer Unreal Online Subsystem (OSS)

## Introduction

The PlayFab Online Subsystem (PF OSS) v2 enables you to make use of PlayFab Multiplayer features like Lobby, Matchmaking, Party and Azure Cognitive Services in your Unreal Engine game.

These features include cross-platform Voice over Internet Protocol (VOIP), cross-network-play, and accessibility features like real-time text chat translation and voice transcription services. It is currently designed for use when developing PC, Xbox, Steam, Nintendo Switch, and Sony PlayStation4/PlayStation5 games. This OSS layer works seamlessly on top of the existing base platform OSS layers (such as the base GDK OSS layer provided by Epic for Xbox and PC Game Pass). PlayFab OSS compliments the base OSS by adding support for PlayFab Lobby, Matchmaking, Party networking and Voice over Internet Protocol (VOIP).

## Getting Started

The quickstart guide for OnlineSubsystemPlayFab can be found at:

[PlayFab Online Subsystem Quickstart](https://docs.microsoft.com/gaming/playfab/features/multiplayer/networking/party-unreal-engine-oss-quickstart)

## Requirements

* Git client with Large File Storage (LFS) support is required to work with this repo

## Version

|OSS Version|Release Note|
|-|-|
Latest Version |2.3.4|
Deprecated Version |2.2.6 and 2.27 may potentially crash the game. **Please upgrade to the latest version to ensure the bug fix is included.**|

## Compatibility

|Unreal Engine version|5.4|
|-|-|
[PlayFab Multiplayer SDK](https://learn.microsoft.com/en-us/gaming/playfab/features/multiplayer/lobby/lobby-matchmaking-sdks/lobby-matchmaking-sdks) | v1.1.5+ (Recommended v1.6.2+ for GDK, Win64+Steam, Nintendo Switch, and Sony PS4™/PS5™)
[PlayFab Party SDK](https://learn.microsoft.com/en-us/gaming/playfab/features/multiplayer/networking/party-sdks) | v1.7.8+ (Recommended v1.8.0+ for GDK and Win64+Steam, v1.8.5+ for Nintendo Switch, and Sony PS4™/PS5™)
Platforms|GDK, Win64+Steam, Nintendo Switch, Sony PS4™/PS5™ (See which version of your platform is supported at [here](https://learn.microsoft.com/en-us/gaming/playfab/features/multiplayer/networking/party-unreal-engine-oss-overview#which-versions-of-the-microsoft-game-development-kit-gdk-are-supported))

For users on previous versions of Unreal Engine (UE4.27+, 5.0, 5.1, and 5.2) who wish to use PlayFab Online Subsystem, please view [PlayFab Online Subsystem Overview](https://learn.microsoft.com/en-us/gaming/playfab/features/multiplayer/networking/party-unreal-engine-oss-overview#which-versions-of-unreal-engine-are-supported) for more details.
If you have any questions, please feel free to open a [Github issue](https://github.com/PlayFab/PlayFabMultiplayerUnreal/issues/new) or open a [support ticket](https://playfab.com/contact/).
## Important Note
#### UE5.4

When using the PlayFab Online Subsystem in UE5.4 GDK, you may encounter the following runtime error: 
`Runtime dependency Party.dll is configured to be staged from C:\Program Files (x86)\Microsoft GDK\<version>\Party.dll and \Engine\Plugins\Online\OnlineSubsystemPlayFab\Platforms\GDK\Redist\Party.dll`


To resolve this issue, follow the steps below:

1. Modify `\Engine\Platforms\GDK\Plugins\Online\OnlineSubsystemGDK\OnlineSubsystemGDK.uplugin` by setting `PlayFabParty` to disabled:
    ```json
    {
        "Name": "PlayFabParty",
        "Enabled": false
    }
    ```

2. Update `\Engine\Platforms\GDK\Plugins\Online\OnlineSubsystemGDK\Source\OnlineSubsystemGDK.Build.cs` by commenting out the inclusion of `PlayFabParty`:
    ```csharp
    if (Target.bCompileAgainstEngine)
    {
        //PublicDependencyModuleNames.Add("PlayFabParty");
    }
    ```

These updates should resolve the staging conflict for `Party.dll`.

#### UE5.3
The WinGDK version of UE5.3 is known to have an issue related to HTTP requests. Epic Games has recognized this problem and addressed it in version 5.4. To work around this issue in UE5.3, you can launch your game using the `-UseWinHttpGDK` argument to switch to WinHTTP instead of xCurl.

The PlayFab Online Subsystem (PF OSS) v2 is currently generally available (GA) for GDK and Win64+Steam, Nintendo Switch, and Sony PS4™/PS5™.

For platform certification and shipping to retail users, games must use the generally available (GA) release of the SDKs and engine plugins that will become available in the coming months.

For games shipping to Xbox console and PC Game Pass program before September 2022, it is recommended that games use the base Online Subsystem (OSS) for GDK provided by Epic for multiplayer integration (backed by Xbox Live multiplayer services) and the PlayFab Online Subsystem (PF OSS) v1.x for Party networking and VOIP available on the [GDK download website](https://aka.ms/gdkdl).

"PlayStation" is a registered trademark or trademark of Sony Interactive Entertainment Inc.<br />
"PS4" is a registered trademark or trademark of Sony Interactive Entertainment Inc.<br />
"PS5" is a registered trademark or trademark of Sony Interactive Entertainment Inc.
