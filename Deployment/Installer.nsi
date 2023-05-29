!include "MUI.nsh"
!include "WinMessages.nsh"

# Define consts
!define PRODUCT_FULLNAME "KakaoTalk ADGuard"
!define PRODUCT_NAME "KakaoTalkADGuard"
!define PRODUCT_VERSION "1.0.0.0"
!define PRODUCT_PUBLISHER "loopback.kr"
!define PRODUCT_DIR_ROOT_REGKEY "HKCU"
!define PRODUCT_DIR_REGKEY "Software\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKCU"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define MUI_ICON "res/logo.ico"
!define MUI_UNICON "res/logo.ico"

# Pages
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PRODUCT_NAME}.exe"
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
ShowInstDetails show
ShowUnInstDetails show

Function .onInit
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    MessageBox MB_OK|MB_ICONEXCLAMATION "${PRODUCT_FULLNAME} is running. Please close it first" /SD IDOK
    Abort
    notRunning:
FunctionEnd

Section "Installer Section"
    SetOutPath $INSTDIR
    File "${PRODUCT_NAME}.exe"
    File "RestoreTrayIcon.exe"
    CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_FULLNAME}.lnk" "$INSTDIR\${PRODUCT_NAME}.exe"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\Restore tray icon.lnk" "$INSTDIR\RestoreTrayIcon.exe"
    CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_FULLNAME}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd

Section Uninstall
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    SendMessage $0 ${WM_CLOSE} 0 0
    notRunning:

    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$SMPROGRAMS\${PRODUCT_NAME}"
    RMDir /r "$INSTDIR"
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey ${PRODUCT_DIR_ROOT_REGKEY} "${PRODUCT_DIR_REGKEY}"
    SetAutoClose true
SectionEnd
