:: SPDX-License-Identifier: GPL-v2-or-later
:: 
:: This script installs/upgrades everything needed to compile Inkscape on Windows with one click.
:: It automates the dependency installation described on the "Compiling Inkscape on Windows" wiki page.
:: See the "echo" lines at the start of the script for details.
:: 
:: Limitations:
:: - Only suitable for interactive use because it asks for user input in a few cases (before script exit, and at certain errors).
:: - Error checking is not implemented, the script will often just silently try to continue.
:: - Only for MINGW64 (x86_64) architecture
::
:: To run the script, just double-click on the windows-deps-clickHere.bat file.
:: 
@echo off
echo Automatic installation/update of Inkscape build dependencies for Windows.
echo.
echo This script will download the following software and install it, partly with Administrator rights:
echo - MSYS2 https://msys2.org/
echo - many MSYS2 packages, see ./msys2installdeps.sh
echo - Microsoft DotNet SDK
echo - WiX https://wixtoolset.org/
echo.
echo Do you want to continue? Press Enter to continue. Close the window to abort.
echo.
pause

:: set $CDSHORTPATH to the DOS 8.3 shortpath of the current directory (to get rid of spaces)
FOR %%I IN ("%CD%") DO SET "cdshortpath=%%~sI"

echo "Starting part 2 with Administrator rights..."
:: We don't use -Wait because sometimes, at the end of the powershell script, the PS window closes but the process doesn't exit.
powershell -Command "Start-Process powershell -Verb runAs -ArgumentList '-ExecutionPolicy','bypass','-File','%cdshortpath%\windows-deps-part2.ps1'"

timeout 5
