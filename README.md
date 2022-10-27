---
title: PlayFab Online Subsystem (OSS) Quickstart
description: Guidance on how to use PlayFab Multiplayer in your Unreal Engine 4 Project.
author: Pushpadant Kacha
ms.author: pkacha
ms.date: 04/4/2022
ms.topic: article
ms.prod: playfab
keywords: playfab, multiplayer, networking, unreal, unreal engine, unreal engine 4, middleware
---

# PlayFab Multiplayer Unreal Online Subsystem (OSS)

## Introduction

The PlayFab Online Subsystem (PF OSS) v2 enables you to make use of PlayFab Multiplayer features like Lobby, Matchmaking, Party and Azure Cognitive Services in your Unreal Engine game.

These features include cross-platform Voice over Internet Protocol (VOIP), cross-network-play, and accessibility features like real-time text chat translation and voice transcription services. It is currently designed for use when developing PC, Xbox, Steam, Nintendo Switch, and Sony PlayStation4/PlayStation5 games. This OSS layer works seamlessly on top of the existing base platform OSS layers (such as the base GDK OSS layer provided by Epic for Xbox and PC Game Pass). PlayFab OSS compliments the base OSS by adding support for PlayFab Lobby, Matchmaking, Party networking and Voice over Internet Protocol (VOIP).

## Getting Started

The quickstart guide for OnlineSubsystemPlayFab can be found at:

[PlayFab Online Subsystem](https://docs.microsoft.com/gaming/playfab/features/multiplayer/networking/party-unreal-engine-oss-quickstart)

## Requirements

* Git client with Large File Storage (LFS) support is required to work with this repo

## Version

|OSS version
|-|
|2.2.5

## Compatibility

|Unreal Engine version|4.27|
|-|-|
Multiplayer SDK|v1.1.5+
Platforms|GDK (June 2021), Win64+Steam, Nintendo Switch, Sony PS4™/PS5™

|Unreal Engine version|5.0|
|-|-|
Multiplayer SDK|v1.1.5+
Platforms|GDK (October 2021, Update 5), Win64+Steam, Nintendo Switch, Sony PS4™/PS5™

## Important Note

The PlayFab Online Subsystem (PF OSS) v2 is currently generally available (GA) for GDK and Win64+Steam, Nintendo Switch, and Sony PS4™/PS5™.

For platform certification and shipping to retail users, games must use the generally available (GA) release of the SDKs and engine plugins that will become available in the coming months.

For games shipping to Xbox console and PC Game Pass program before September 2022, it is recommended that games use the base Online Subsystem (OSS) for GDK provided by Epic for multiplayer integration (backed by Xbox Live multiplayer services) and the PlayFab Online Subsystem (PF OSS) v1.x for Party networking and VOIP available on the [GDK download website](https://aka.ms/gdkdl).

## Known Limitations and Issues in the Current Preview Build

- [General] Currently cross-play is only supported between Xbox consoles, PC Game Pass, Steam, Switch, and Sony PS4™/PS5™.  Cross-play support between other platforms is coming soon.

"PlayStation" is a registered trademark or trademark of Sony Interactive Entertainment Inc.<br />
"PS4" is a registered trademark or trademark of Sony Interactive Entertainment Inc.<br />
"PS5" is a registered trademark or trademark of Sony Interactive Entertainment Inc.
