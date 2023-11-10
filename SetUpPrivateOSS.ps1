# This script downloads the private platform components of the OSS, for the platforms the user has access to.
# The binaries and header files are pulled from our NuGet feeds at https://dev.azure.com/PlayFabPrivate/. You can
# find the NuGet config files in the Platforms/[APlatform] folder.
# The private source code is pulled from our Git repositories as submodules.

# Prerequesites: download NuGet.exe and add its path to the PATH environment variable. Have git installed.

param (
	[Parameter(Mandatory=$true)]
	[validateset("Switch", "PlayStation")]
	[string]$Platform
)

$partyNugetFileName = ""
$partyNugetVersion = ""
$mlpNugetFileName = ""
$mlpNugetVersion = ""

function Set-NuGetPackageInfo($localPath, $partyFileName, $partyVersion, $mlpFileName, $mlpVersion)
{
    $packagesXml = Get-Content $localPath\packages.config".xml"
    $packagesXml = $packagesXml -replace "##PARTY##", $partyFileName
    $packagesXml = $packagesXml -replace "##PARTY_VERSION##", $partyVersion
    $packagesXml = $packagesXml -replace "##MULTIPLAYER##", $mlpFileName
    $packagesXml = $packagesXml -replace "##MULTIPLAYER_VERSION##", $mlpVersion
    Set-Content -Path $localPath\packages.config -Value $packagesXml
}

if ($Platform -eq "Switch")
{
	Write-Host "Select Switch SDK version..."
	Write-Host "    1: SDK 14.3.0-shipping"
	Write-Host "    2: SDK 14.3.0-logging"
    Write-Host "    3: SDK 15.3.2-shipping"
    Write-Host "    4: SDK 15.3.2-logging"
    while($True)
    {
        Write-Host -NoNewline "> "
        $key = $Host.UI.RawUI.ReadKey()
        $Selection = $key.Character
        Write-Host ""
        if ($Selection -eq '1')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.Switch-14.3.0-shipping"
            $partyNugetVersion = "1.8.3"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.Switch-14.3.0-shipping"
            $mlpNugetVersion = "1.6.1"
            break
        }
        elseif ($Selection -eq '2')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.Switch-14.3.0-logging"
            $partyNugetVersion = "1.8.3"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.Switch-14.3.0-logging"
            $mlpNugetVersion = "1.6.1"
            break
        }
        elseif ($Selection -eq '3')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.Switch-15.3.2-shipping"
            $partyNugetVersion = "1.8.5"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.Switch-15.3.2-shipping"
            $mlpNugetVersion = "1.6.2"
            break
        }
        elseif ($Selection -eq '4')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.Switch-15.3.2-logging"
            $partyNugetVersion = "1.8.5"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.Switch-15.3.2-logging"
            $mlpNugetVersion = "1.6.2"
            break
        }
        Write-Host "Unknown input"
    }
	Write-Host "Downloading Switch Party and Multiplayer NuGet packages..."
    Write-Host $partyNugetFileName". Downloaded Party SDK version="$partyNugetVersion
    Write-Host $mlpNugetFileName". Downloaded Multiplayer SDK version="$mlpNugetVersion
    Write-Host "If you want to use a higher version of Party and Multiplayer SDK, please checkout to our private repositories and follow https://learn.microsoft.com/en-us/gaming/playfab/features/multiplayer/networking/party-unreal-engine-oss-obtaining-playfab-party-libraries"
	$localPath = ".\Platforms\Switch"
    Set-NuGetPackageInfo $localPath $partyNugetFileName $partyNugetVersion $mlpNugetFileName $mlpNugetVersion
	nuget.exe restore $localPath -ConfigFile $localPath\nuget.config -PackagesDirectory $localPath

	Write-Host "Updating git submodules for private platforms..."
	git submodule update --recursive --init Source/PlatformSpecific/Switch
}
elseif ($Platform -eq "PlayStation")
{
	Write-Host "Select PlayStation4 SDK version..."
	Write-Host "    1: SDK 9.500"
	Write-Host "    2: SDK 10.000"
    Write-Host "    3: SDK 10.500"
    while($True)
    {
        Write-Host -NoNewline "> "
        $key = $Host.UI.RawUI.ReadKey()
        $Selection = $key.Character
        Write-Host ""
        if ($Selection -eq '1')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.PS4-9.500"
            $partyNugetVersion = "1.7.15"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.PS4-9.500"
            $mlpNugetVersion = "1.4.3"
            break
        }
        elseif ($Selection -eq '2')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.PS4-10.000"
            $partyNugetVersion = "1.8.5"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.PS4-10.000"
            $mlpNugetVersion = "1.6.1"
            break
        }
        elseif ($Selection -eq '3')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.PS4-10.500"
            $partyNugetVersion = "1.8.5"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.PS4-10.500"
            $mlpNugetVersion = "1.6.2"
            break
        }
        Write-Host "Unknown input"
    }
	Write-Host "Downloading PlayStation4 Party and Multiplayer NuGet packages..."
    Write-Host $partyNugetFileName". Downloaded Party SDK version="$partyNugetVersion
    Write-Host $mlpNugetFileName". Downloaded Multiplayer SDK version="$mlpNugetVersion
    Write-Host "If you want to use a higher version of Party and Multiplayer SDK, please checkout to our private repositories and follow https://learn.microsoft.com/en-us/gaming/playfab/features/multiplayer/networking/party-unreal-engine-oss-obtaining-playfab-party-libraries"
	$localPath = ".\Platforms\PS4"
    Set-NuGetPackageInfo $localPath $partyNugetFileName $partyNugetVersion $mlpNugetFileName $mlpNugetVersion
	nuget.exe restore $localPath -ConfigFile $localPath\nuget.config -PackagesDirectory $localPath
	
    $Selection = ''
    if (-not $Version -eq '')
    {
        $Selection = $Version
    }
	Write-Host "Select PlayStation5 SDK version..."
	Write-Host "    1: SDK 5.000"
	Write-Host "    2: SDK 6.000"
    Write-Host "    3: SDK 7.000"
    while($True)
    {
        Write-Host -NoNewline "> "
        $key = $Host.UI.RawUI.ReadKey()
        $Selection = $key.Character
        Write-Host ""
        if ($Selection -eq '1')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.PS5-5.000"
            $partyNugetVersion = "1.7.15"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.PS5-5.000"
            $mlpNugetVersion = "1.4.3"
            break
        }
        elseif ($Selection -eq '2')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.PS5-6.000"
            $partyNugetVersion = "1.8.5"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.PS5-6.000"
            $mlpNugetVersion = "1.6.1"
            break
        }
        elseif ($Selection -eq '3')
        {
            $partyNugetFileName = "Microsoft.PlayFab.PlayFabParty.Cpp.PS5-7.000"
            $partyNugetVersion = "1.8.5"
            $mlpNugetFileName = "Microsoft.PlayFab.Multiplayer.Cpp.PS5-7.000"
            $mlpNugetVersion = "1.6.2"
            break
        }
        Write-Host "Unknown input"
    }
	Write-Host "Downloading PlayStation5 Party and Multiplayer NuGet packages..."
    Write-Host $partyNugetFileName". Downloaded Party SDK version="$partyNugetVersion
    Write-Host $mlpNugetFileName". Downloaded Multiplayer SDK version="$mlpNugetVersion
    Write-Host "If you want to use a higher version of Party and Multiplayer SDK, please checkout to our private repositories and follow https://learn.microsoft.com/en-us/gaming/playfab/features/multiplayer/networking/party-unreal-engine-oss-obtaining-playfab-party-libraries"
    $localPath = ".\Platforms\PS5"
    Set-NuGetPackageInfo $localPath $partyNugetFileName $partyNugetVersion $mlpNugetFileName $mlpNugetVersion
	nuget.exe restore $localPath -ConfigFile $localPath\nuget.config -PackagesDirectory $localPath

	Write-Host "Updating git submodules for private platforms..."
	git submodule update --recursive --init Source/PlatformSpecific/PlayStation

	Write-Host "Patching PlayStation platforms patch..."
	git apply --reject --whitespace=fix Source/PlatformSpecific/PlayStation/playstation.patch
}
