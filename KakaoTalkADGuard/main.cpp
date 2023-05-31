#include "framework.h"
#include "KakaoTalkADGuard.h"

// Global variables
HINSTANCE       hInst;
LPWSTR          szCmdLine = 0;
WCHAR           szTitle[MAX_LOADSTRING];
WCHAR           szWindowClass[MAX_LOADSTRING];
UINT            updateRate = 100;
BOOL            autoStartup = false;
BOOL            hideTrayIcon = false;
NOTIFYICONDATA  nid = {sizeof(nid)};

// Forward-declaration
ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
BOOL             CheckMultipleExecution(HINSTANCE hInst, HWND hWnd, WCHAR szWindowClass[MAX_LOADSTRING]);
BOOL             HideTrayIcon(HINSTANCE hInst, HWND hWnd, NOTIFYICONDATA nid);
BOOL             ToggleStartup(HWND hWnd);
BOOL             CheckStartup(HINSTANCE hInst, HWND hWnd);
BOOL             CreateTrayIcon(HWND hWnd, NOTIFYICONDATA* nid);
BOOL             DeleteTrayIcon(NOTIFYICONDATA nid);
VOID             ShowContextMenu(HWND hwnd, POINT pt);
BOOL             ShowNewUpdateBalloon();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK    TimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTimer);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	// Parse command-line
	if (lpCmdLine != L"")
		szCmdLine = lpCmdLine;
	
	// Load resources
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_KAKAOTALKADGUARD, szWindowClass, MAX_LOADSTRING);

	// Initialize Window
	MyRegisterClass(hInstance);
	if (!InitInstance(hInstance, nCmdShow)) { return FALSE; }

	// Message loop
	MSG msg; while (GetMessage(&msg, nullptr, 0, 0)) { // Wait for new message
		TranslateMessage(&msg); // Translate WM_KEYDOWN to WM_CHAR
		DispatchMessage(&msg); // Dispatch to WndProc
	} return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOGO));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_LOGO));
	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	hInst = hInstance;
	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) { return FALSE; }
	/*ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);*/
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { // Called by kernel
	static HANDLE hTimer;
	static WCHAR appName[64];
	BOOL bClose = FALSE;

	switch (message) {
	case WM_CREATE:
		bClose = CheckMultipleExecution(hInst, hWnd, szWindowClass);
		hTimer = (HANDLE) SetTimer(hWnd, 1, updateRate, (TIMERPROC) TimerProc);
		CheckStartup(hInst, hWnd);
		if (!hideTrayIcon and !bClose) {
			CreateTrayIcon(hWnd, &nid);
		}
		break;
	case WM_NOTIFYCALLBACK:
		switch (LOWORD(lParam)) {
		case NIN_SELECT:
		case WM_CONTEXTMENU:
		{
			POINT const pt = {LOWORD(wParam), HIWORD(wParam)};
			ShowContextMenu(hWnd, pt);
		}
		break;
		}
		break;
	case WM_RECHECK:
		CheckStartup(hInst, hWnd);
		if (!hideTrayIcon) {
			CreateTrayIcon(hWnd, &nid);
		}
		break;
	case WM_INITMENU:
		if (autoStartup)
			CheckMenuItem((HMENU) wParam, IDM_STARTONSYSTEMSTARTUP, MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem((HMENU) wParam, IDM_STARTONSYSTEMSTARTUP, MF_BYCOMMAND | MF_UNCHECKED);

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDM_HIDETRAYICON:
			HideTrayIcon(hInst, hWnd, nid);
			break;
		case IDM_STARTONSYSTEMSTARTUP:
			ToggleStartup(hWnd);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_DESTROY:
		KillTimer(hWnd, 1);
		DeleteTrayIcon(nid);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

BOOL CheckStartup(HINSTANCE hInst, HWND hWnd) {
	// Check autoStartup
	HKEY key; DWORD dwDisp;
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_RUN, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);

	if (RegQueryValueExW(key, L"KakaoTalkADGuard", 0, NULL, 0, NULL) == NO_ERROR) {
		autoStartup = true;
	} else {
		autoStartup = false;
	}
	RegCloseKey(key);
	SendMessage(hWnd, WM_INITMENU, 0, 0);

	// Check HideTrayIcon
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &key, &dwDisp);
	DWORD dwType = REG_DWORD;
	DWORD dwValue = 0;
	DWORD dwDataSize = sizeof(DWORD);
	DWORD ret = RegQueryValueExW(key, L"HideTrayIcon", 0, &dwType, (LPBYTE) &dwValue, &dwDataSize);
	if (dwValue) {
		hideTrayIcon = TRUE;
	} else {
		hideTrayIcon = FALSE;
	}
	RegCloseKey(key);
	return 0;
}

BOOL ToggleStartup(HWND hWnd) {
	HKEY key; DWORD dwDisp;
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_RUN, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);

	if (RegQueryValueExW(key, L"KakaoTalkADGuard", 0, NULL, 0, NULL) == NO_ERROR) {
		RegDeleteValueW(key, L"KakaoTalkADGuard");
		autoStartup = false;
	} else {
		WCHAR szFileName[MAX_PATH];
		GetModuleFileName(NULL, szFileName, MAX_PATH);
		RegSetValueExW(key, L"KakaoTalkADGuard", 0, REG_SZ, (LPBYTE) szFileName, (lstrlenW(szFileName) + 1) * sizeof(WCHAR));
		autoStartup = true;
	}
	RegCloseKey(key);
	return 0;
}

BOOL HideTrayIcon(HINSTANCE hInst, HWND hWnd, NOTIFYICONDATA nid) {
	WCHAR msgboxHideTray[MAX_LOADSTRING];
	LoadStringW(hInst, IDS_MSGBOX_HIDETRAY, msgboxHideTray, MAX_LOADSTRING);
	MessageBox(hWnd, msgboxHideTray, NULL, MB_ICONWARNING);
	HKEY key; DWORD dwDisp;
	LSTATUS ret;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &dwDisp) == ERROR_SUCCESS) {
		DWORD value = 1;
		ret = RegSetValueExW(key, L"HideTrayIcon", 0, REG_DWORD, (const BYTE*) &value, sizeof(value));
	}
	RegCloseKey(key);
	DeleteTrayIcon(nid);
	return 0;
}

BOOL CheckMultipleExecution(HINSTANCE hInst, HWND hWnd, WCHAR szWindowClass[MAX_LOADSTRING]) {
	CreateMutex(NULL, TRUE, szWindowClass);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		WCHAR msgboxIsRunning[MAX_LOADSTRING];
		LoadStringW(hInst, IDS_MSGBOX_ISRUNNING, msgboxIsRunning, MAX_LOADSTRING);
		MessageBox(hWnd, msgboxIsRunning, NULL, MB_ICONWARNING);
		PostQuitMessage(0);
		return 1;
	}
	return 0;
}

BOOL ShowNewUpdateBalloon() {
	NOTIFYICONDATA nid = {sizeof(nid)};
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_INFO;
	LoadStringW(hInst, IDS_NEWUPDATE_TITLE, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
	LoadStringW(hInst, IDS_NEWUPDATE_CONTENT, nid.szInfo, ARRAYSIZE(nid.szInfo));
	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

BOOL CreateTrayIcon(HWND hWnd, NOTIFYICONDATA* nid) {
	nid->hWnd = hWnd;
	nid->uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
	LoadString(hInst, IDS_APP_NAME, nid->szTip, ARRAYSIZE(nid->szTip));
	nid->hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGO));
	nid->uCallbackMessage = WM_NOTIFYCALLBACK;
	Shell_NotifyIcon(NIM_ADD, nid);
	nid->uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, nid);
}

BOOL DeleteTrayIcon(NOTIFYICONDATA nid) {
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd, POINT pt) {
	HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_TRAY_CONTEXTMENU));
	HMENU hSubMenu = GetSubMenu(hMenu, 0);
	SetForegroundWindow(hwnd); // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
	// respect menu drop alignment
	UINT uFlags = TPM_RIGHTBUTTON;
	if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
		uFlags |= TPM_RIGHTALIGN;
	} else {
		uFlags |= TPM_LEFTALIGN;
	}
	TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
	DestroyMenu(hMenu);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lparam) {
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if ((DWORD) lparam == pid) {
		return FALSE; // found
	} else {
		return TRUE; // continue
	}
}

VOID CALLBACK TimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTimer) {
	switch (idEvent) {
	case 1: // Remove KakaoTalk ADs
		// Find handles
		RECT RectKakaoTalkMain;
		HWND hKakaoTalkMain = FindWindow(L"EVA_Window_Dblclk", L"카카오톡");
		HWND hChildWnd = FindWindowEx(hKakaoTalkMain, NULL, L"EVA_ChildWindow", NULL);
		HWND hBannerWnd = FindWindowEx(hKakaoTalkMain, NULL, L"BannerAdWnd", NULL);
		HWND hLockBannerWnd = FindWindowEx(hKakaoTalkMain, NULL, L"EVA_ChildWindow_Dblclk", NULL);
		HWND hPopupWnd = FindWindow(L"RichPopWnd", L"");
		
		// Sanity check for Popup AD
		DWORD pid_main = 0;
		DWORD pid_popup = 0;
		GetWindowThreadProcessId(hKakaoTalkMain, &pid_main);
		GetWindowThreadProcessId(hPopupWnd, &pid_popup);

		// Remove ADs
		GetWindowRect(hKakaoTalkMain, &RectKakaoTalkMain);
		ShowWindow(hBannerWnd, SW_HIDE);
		SetWindowPos(hChildWnd, HWND_BOTTOM, 0, 0, (RectKakaoTalkMain.right - RectKakaoTalkMain.left), (RectKakaoTalkMain.bottom - RectKakaoTalkMain.top - 32), SWP_NOMOVE);
		SetWindowPos(hLockBannerWnd, HWND_BOTTOM, 0, 0, (RectKakaoTalkMain.right - RectKakaoTalkMain.left), (RectKakaoTalkMain.bottom - RectKakaoTalkMain.top - 32), SWP_NOMOVE);
		if (pid_main == pid_popup)
			ShowWindow(hPopupWnd, SW_HIDE);
		break;
	}
}