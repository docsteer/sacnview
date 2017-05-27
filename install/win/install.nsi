; Installation Script for sACNView
; Copyright 2016 Tom Barthel-Steer
; http://www.tomsteer.net
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

!define PRODUCT_NAME "sACNView"
!define PRODUCT_VERSION "2.0.0 RC 1"
!define PRODUCT_PUBLISHER "Tom Barthel-Steer"
!define PRODUCT_WEB_SITE "http://www.tomsteer.net"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\sACNView.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

;..................................................................................................
;Following two definitions required. Uninstall log will use these definitions.
;You may use these definitions also, when you want to set up the InstallDirRagKey,
;store the language selection, store Start Menu folder etc.
;Enter the windows uninstall reg sub key to add uninstall information to Add/Remove Programs also.

!define INSTDIR_REG_ROOT "HKLM"
!define INSTDIR_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
;..................................................................................................

!include MUI.nsh
!include AdvUninstLog.nsh

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "..\..\res\icon.ico"

Name "${PRODUCT_NAME}"
OutFile "${PRODUCT_NAME}_${PRODUCT_VERSION}.exe"
ShowInstDetails show
ShowUninstDetails show
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "InstallDir"

!insertmacro INTERACTIVE_UNINSTALL

!insertmacro MUI_PAGE_WELCOME

; Check for Admin rights
Section CheckAdmin
	DetailPrint "Checking Admin Rights"
	System::Call "kernel32::GetModuleHandle(t 'shell32.dll') i .s"
	System::Call "kernel32::GetProcAddress(i s, i 680) i .r0"
	System::Call "::$0() i .r0"

	IntCmp $0 0 isNotAdmin isNotAdmin isAdmin
isNotAdmin:
	DetailPrint "Missing Administrator Rights !!!"
	messageBox MB_OK "You do not have Administrator rights on this computer.$\r$\r\
Please log in as an administrator to install sACNView."
	quit
isAdmin:
	DetailPrint "Administrator Rights granted"
SectionEnd

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN $INSTDIR\${PRODUCT_NAME}.exe
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"


Section "Main Application" sec01

	SetOutPath '$INSTDIR'

;After set the output path open the uninstall log macros block and add files/dirs with File /r
;This should be repeated every time the parent output path is changed either within the same
;section, or if there are more sections including optional components.
	!insertmacro UNINSTALL.LOG_OPEN_INSTALL

	File /r "..\deploy\"

;Once required files/dirs added and before change the parent output directory we need to
;close the opened previously uninstall log macros block.
	!insertmacro UNINSTALL.LOG_CLOSE_INSTALL

	CreateDirectory '$SMPROGRAMS\${PRODUCT_NAME}'
	CreateShortcut '$SMPROGRAMS\${PRODUCT_NAME}\sACNView.lnk' '$INSTDIR\sACNView.exe'
	;create shortcut for uninstaller always use ${UNINST_EXE} instead of uninstall.exe
	CreateShortcut '$SMPROGRAMS\${PRODUCT_NAME}\uninstall.lnk' '${UNINST_EXE}'

	WriteRegStr ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "InstallDir" "$INSTDIR"
	WriteRegStr ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "DisplayName" "${PRODUCT_NAME}"
	;Same as create shortcut you need to use ${UNINST_EXE} instead of anything else.
	WriteRegStr ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}" "UninstallString" "${UNINST_EXE}"

SectionEnd


Function .onInit

        ;prepare log always within .onInit function
        !insertmacro UNINSTALL.LOG_PREPARE_INSTALL

FunctionEnd


Function .onInstSuccess

         ;create/update log always within .onInstSuccess function
         !insertmacro UNINSTALL.LOG_UPDATE_INSTALL

FunctionEnd

#######################################################################################

Section UnInstall

         ;begin uninstall, especially for MUI could be added in UN.onInit function instead
         ;!insertmacro UNINSTALL.LOG_BEGIN_UNINSTALL

         ;uninstall from path, must be repeated for every install logged path individual
         !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR"

         ;end uninstall, after uninstall from all logged paths has been performed
         !insertmacro UNINSTALL.LOG_END_UNINSTALL

        Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
        Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"
        RmDir "$SMPROGRAMS\${PRODUCT_NAME}"

        DeleteRegKey /ifempty ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}"

SectionEnd


Function UN.onInit

         ;begin uninstall, could be added on top of uninstall section instead
         !insertmacro UNINSTALL.LOG_BEGIN_UNINSTALL

FunctionEnd