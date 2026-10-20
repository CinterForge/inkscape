; SPDX-License-Identifier: GPL-2.0-or-later
; development@maxgaukler.de 2024

Section -runAlways_RemoveOldVersions
	; Check for previous Inkscape installation and raise error to avoid an inconsistent result.
	; Currently we assume that the installation directory is not changed.
	
	IfFileExists "$INSTDIR\bin\inkscape.exe" previousInstallFound noPreviousInstallFound
	
	previousInstallFound:
	
	IfSilent 0 +2 ; in silent mode, just abort with error
	Abort
	; in non-silent mode, try to uninstall previous installations
	DetailPrint "Uninstalling previous version (MSI)"
	nsExec::Exec '"wmic" product where Name="Inkscape" uninstall'
	; Note: Uninstalling via EXE should normally be handled by CPack/NSIS, but we try it anyway
	nsExec::Exec '"$INSTDIR\Uninstall.exe" /S _?=$INSTDIR'
	DetailPrint "done"
	
	; Now check again if Inkscape is already installed
	IfFileExists "$INSTDIR\bin\inkscape.exe" previousInstallStillFound previousInstallSuccessfullyRemoved
	
	previousInstallStillFound:
	; We were unable to remove it. Error.
	MessageBox MB_OK|MB_ICONEXCLAMATION "A previous installation of Inkscape was found. Please uninstall it via system settings."
	Abort
	
	previousInstallSuccessfullyRemoved:
	DetailPrint "Previous version was uninstalled successfully."
	
	noPreviousInstallFound:
SectionEnd

;EOF