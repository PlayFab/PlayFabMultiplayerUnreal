# This script downloads the private platform components of the OSS, for the platforms the user has access to.
# The binaries and header files are pulled from our NuGet feeds at https://dev.azure.com/PlayFabPrivate/. You can
# find the NuGet config files in the Platforms/[APlatform] folder.
# The private source code is pulled from our Git repositories as submodules.

# Prerequesites: download NuGet.exe and add its path to the PATH environment variable. Have git installed.

Write-Host "Downloading Party and Multiplayer NuGet packages..."

$localPath = ".\Platforms\Switch"
nuget.exe restore $localPath -ConfigFile $localPath\nuget.config -PackagesDirectory $localPath

# Check that the Nugets have been copied. We look for a binary file as a proxy.
$mlpBinary = Get-ChildItem $localPath -name -recurse libPlayFabMultiplayer.nro
$partyBinary = Get-ChildItem $localPath -name -recurse Party.nro
if (!$mlpBinary -or !$partyBinary) {
	Write-Host "There was an error downloading the NuGet packages."
	return
}

Write-Host "NuGet package downloads successful."

Write-Host "Updating git submodules for private platforms..."
git submodule update --recursive --init

# Check that the submodule has been copied.
$platformSpecificPath = ".\Source\PlatformSpecific\Switch"
$chatFile = Get-ChildItem $platformSpecificPath -name -recurse ChatPermissionTracking.Switch.cpp
$networkFile = Get-ChildItem $platformSpecificPath -name -recurse NetworkManager.Switch.cpp
$identityFile = Get-ChildItem $platformSpecificPath -name -recurse OnlineIdentityInterfacePlayFab.Switch.cpp
if (!$chatFile -or !$networkFile -or !$identityFile) {
	Write-Host "There was an error restoring the submodules."
	return
}

Write-Host "Submodules updated successfully."