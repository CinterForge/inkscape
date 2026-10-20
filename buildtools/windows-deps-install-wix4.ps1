# SPDX-License-Identifier: GPL-2.0-or-later
#
# Installation of WiX v4 toolset for building Windows .MSI packages.
#
# This script is used both by the CI (.gitlab-ci.yml) and for manual installation (main script: windows-deps-clickHere.bat)

# Install DotNet runtime
winget  install --accept-package-agreements --accept-source-agreements Microsoft.DotNet.SDK.8

# determine path to MSYS. For normal developers, it is C:\msys64. For CI it is C:\%IDW_NAME%. Other values can be set with the MSYS environment variable.
$MSYS = $Env:MSYS
if (-not $MSYS) {
	if ($Env:IDW_NAME) {
		$MSYS = "C:\" + $Env:IDW_NAME
	} else {
		$MSYS = "C:\msys64"
	}
}

# install WiX in a path where MSYS finds it
$WIXPATH = $MSYS + "\usr\local\bin\"
$Env:DOTNET_CLI_TELEMETRY_OPTOUT = "1"
# Note: we use an explicit path to dotnet.exe because PATH is not reloaded by winget after the installation
& "$Env:ProgramFiles\dotnet\dotnet" tool install wix --version 4.0.4 --tool-path $WIXPATH
& "$WIXPATH\wix" extension add --global WixToolset.UI.wixext/4.0.4
