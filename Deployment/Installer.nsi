!include "MUI.nsh"
!include "WinMessages.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"

# Define consts
!define PRODUCT_FULLNAME "KakaoTalk ADGuard"
!define PRODUCT_NAME "KakaoTalkADGuard"
!define PRODUCT_COMMENTS "AD removal tool for Windows KakaoTalk"
!define PRODUCT_VERSION "1.0.0.6"
!define BUILD_ARCH "x64"
!define PRODUCT_PUBLISHER "loopback.kr"
!define PRODUCT_REG_ROOTKEY "HKCU"
!define PRODUCT_DIR_REGKEY "Software\${PRODUCT_NAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define MUI_ICON "res/install.ico"
!define MUI_UNICON "res/uninstall.ico"

# Pages
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PRODUCT_NAME}.exe"
!define MUI_FINISHPAGE_RUN_PARAMETERS "--startup"
!define MUI_FINISHPAGE_RUN_TEXT "Run ${PRODUCT_FULLNAME}"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

OutFile "${PRODUCT_NAME} ${PRODUCT_VERSION}.exe"
InstallDirRegKey HKCU "SOFTWARE\${PRODUCT_NAME}" "InstallPath"
InstallDir "$APPDATA\${PRODUCT_NAME}"
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
BrandingText /TRIMRIGHT "${PRODUCT_NAME}"
RequestExecutionLevel user
ShowInstDetails show
ShowUnInstDetails show

VIProductVersion "${PRODUCT_VERSION}"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "FileDescription" "${PRODUCT_FULLNAME} Setup"
VIAddVersionKey "ProductName" "${PRODUCT_FULLNAME}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
# VIAddVersionKey "LegalTrademarks" "Test Application is a trademark of Fake company"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2024 loopback.kr"
# VIAddVersionKey "OriginalFilename" "${PRODUCT_NAME} ${PRODUCT_VERSION}.exe"

Function .onInit
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    SendMessage $0 ${WM_CLOSE} 0 0
    ; MessageBox MB_OK|MB_ICONEXCLAMATION "${PRODUCT_FULLNAME} is running. Please close it first." /SD IDOK
    ; Abort
    notRunning:
FunctionEnd

Section "Installer Section"
    SetOutPath $INSTDIR
    ${If} ${RunningX64}
        File "..\Release\x64\${PRODUCT_NAME}.exe"
    ${Else}
        File "..\Release\win32\${PRODUCT_NAME}.exe"
    ${EndIf}
    ; File "RestoreTrayIcon.exe"
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_FULLNAME}.lnk" "$INSTDIR\${PRODUCT_NAME}.exe" "--startup"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\Restore tray icon.lnk" "$INSTDIR\${PRODUCT_NAME}.exe" "--restore_tray"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_FULLNAME}"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$\"$INSTDIR\${PRODUCT_NAME}.exe$\""
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "Comments" "${PRODUCT_COMMENTS}"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
    # WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "Contact" "mailto:hyunseoki@outlook.kr"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "HelpLink" "https://github.com/loopback-kr/KakaoTalkADGuard/issues"
    # WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "https://github.com/loopback-kr/KakaoTalkADGuard/issues"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "URLUpdateInfo" "https://github.com/loopback-kr/KakaoTalkADGuard#release-notes"
    WriteRegStr ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegDWORD ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd

Section Uninstall
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    SendMessage $0 ${WM_CLOSE} 0 0
    notRunning:

    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$SMPROGRAMS\${PRODUCT_NAME}"
    RMDir /r "$INSTDIR"
    DeleteRegKey ${PRODUCT_REG_ROOTKEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey ${PRODUCT_REG_ROOTKEY} "${PRODUCT_DIR_REGKEY}"
    DeleteRegValue ${PRODUCT_REG_ROOTKEY} "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\" "${PRODUCT_NAME}"
    SetAutoClose true
SectionEnd
