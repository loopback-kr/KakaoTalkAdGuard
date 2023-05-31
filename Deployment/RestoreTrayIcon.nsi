# Define consts
!define PRODUCT_FULLNAME "KakaoTalk ADGuard"
!define PRODUCT_NAME "KakaoTalkADGuard"
!define PRODUCT_VERSION "1.0.0.2"
!define PRODUCT_PUBLISHER "loopback.kr"
!define PRODUCT_DIR_ROOT_REGKEY "HKCU"
!define PRODUCT_DIR_REGKEY "Software\${PRODUCT_NAME}"

OutFile "RestoreTrayIcon.exe"
Name "${PRODUCT_NAME} RestoreTrayIcon"
AutoCloseWindow true

Function .onInit
    # Set registry
    WriteRegDWORD ${PRODUCT_DIR_ROOT_REGKEY} "${PRODUCT_DIR_REGKEY}" "HideTrayIcon" 0
    # Send Message for refresh
    FindWindow $0 "${PRODUCT_NAME}"
    StrCmp $0 0 notRunning
    SendMessage $0 32770 0 0
    notRunning:
    # Messagebox
    MessageBox MB_OK|MB_ICONINFORMATION "${PRODUCT_FULLNAME} tray icon is enabled."
FunctionEnd

Function .onGUIInit
    abort
FunctionEnd

Section
SectionEnd