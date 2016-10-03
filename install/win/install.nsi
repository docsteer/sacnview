; Installation Script for sACNView

; Definitions
!define PRODUCT_NAME "sACNView"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Tom Barthel-Steer"
!define PRODUCT_WEB_SITE "http://www.tomsteer.net"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\sACNView.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "..\..\res\icon.ico"

; Welcome page
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
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN $INSTDIR\sACNView.exe
!insertmacro MUI_PAGE_FINISH

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME}_${PRODUCT_VERSION}.exe"
InstallDir "$PROGRAMFILES\sACNView"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show

Section "Application" SEC01
    SetShellVarContext all
    SetOutPath "$INSTDIR"
    SetOverwrite on
    File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\D3Dcompiler_43.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\libEGL.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\libgcc_s_dw2-1.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\libstdc++-6.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\libwinpthread-1.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\opengl32sw.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\Qt5Core.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\Qt5Gui.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\Qt5Network.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\Qt5Svg.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\Qt5Widgets.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\Qt5Widgets.dll"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\release\sACNView.exe"
	WriteUninstaller $INSTDIR\Uninstall.exe
	;CreateDirectory "$INSTDIR\bearer"
	;CreateDirectory "$INSTDIR\iconengines"
	;CreateDirectory "$INSTDIR\imageformats"
	CreateDirectory "$INSTDIR\platforms"
	SetOutPath "$INSTDIR\platforms"
	File "..\..\..\build-sACNView-Desktop_Qt_5_5_1_MinGW_32bit-Release\deployment\platforms\qwindows.dll"
	;CreateDirectory "$INSTDIR\translations"
	
	CreateDirectory "$SMPROGRAMS\sACNView"
    CreateShortCut "$SMPROGRAMS\sACNView\sACNView.lnk" "$INSTDIR\sACNView.exe"
    CreateShortCut "$DESKTOP\sACNView.lnk" "$INSTDIR\sACNView.exe"
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  SetShellVarContext all
  Delete "$INSTDIR\\D3Dcompiler_43.dll"
  Delete "$INSTDIR\libEGL.dll"
  Delete "$INSTDIR\libgcc_s_dw2-1.dll"
  Delete "$INSTDIR\libstdc++-6.dll"
  Delete "$INSTDIR\libwinpthread-1.dll"
  Delete "$INSTDIR\opengl32sw.dll"
  Delete "$INSTDIR\Qt5Core.dll"
  Delete "$INSTDIR\Qt5Gui.dll"
  Delete "$INSTDIR\Qt5Network.dll"
  Delete "$INSTDIR\Qt5Svg.dll"
  Delete "$INSTDIR\Qt5Widgets.dll"
  Delete "$INSTDIR\sACNView.exe"
  Delete "$INSTDIR\Uninstall.exe"
  Delete "$INSTDIR\platforms\qwindows.dll"
  RMDir "$INSTDIR\platforms"
  RMDir $INSTDIR
    
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd

