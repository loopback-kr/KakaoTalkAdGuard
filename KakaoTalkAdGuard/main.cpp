#include "framework.h"
#include "KakaoTalkAdGuard.h"
#include <chrono>
#include <string>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// Global variables
HINSTANCE       hInst;
LPWSTR          szCmdLine = 0;
WCHAR           szTitle[MAX_LOADSTRING];
WCHAR           szWindowClass[MAX_LOADSTRING];
UINT            updateRate = 100;
BOOL            autoStartup = false;
BOOL            bCheckUpdate = false;
BOOL            hideTrayIcon = false;
NOTIFYICONDATA  nid = {sizeof(nid)};
BOOL            bPortable = true;
BOOL            bHideMainPannelAd = false;
BOOL            bUpdateBannerAdRegistry = false;

// Forward-declaration
ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
BOOL             CheckMultipleExecution(HINSTANCE hInst, HWND hWnd, WCHAR szWindowClass[MAX_LOADSTRING]);
BOOL             HideTrayIcon(HINSTANCE hInst, HWND hWnd, NOTIFYICONDATA nid);
BOOL             ToggleStartup(HWND hWnd);
BOOL             ToggleCheckUpdate(HWND hWnd);
BOOL             ToggleHideMainPannelAd(HWND hWnd);
BOOL             ToggleUpdateBannerAdRegistry(HWND hWnd);
BOOL             CheckStartup(HINSTANCE hInst, HWND hWnd);
BOOL             CreateTrayIcon(HWND hWnd, NOTIFYICONDATA* nid);
BOOL             DeleteTrayIcon(NOTIFYICONDATA nid);
VOID             ShowContextMenu(HWND hwnd, POINT pt);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK    TimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTimer);
std::wstring     GetLatestReleaseTag(const std::wstring& owner, const std::wstring& repo);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	// Parse command-line
	if (lpCmdLine != L"") {
		szCmdLine = lpCmdLine;
	}
	
	// Load resources
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	if (lstrcmpW(szCmdLine, L"--restore_tray") == 0) {
		LoadStringW(hInstance, IDC_KAKAOTALKADGUARD_RESTORETRAY, szWindowClass, MAX_LOADSTRING);
	} else {
		LoadStringW(hInstance, IDC_KAKAOTALKADGUARD, szWindowClass, MAX_LOADSTRING);
	}

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
	BOOL bRestoretray = FALSE;

	switch (message) {
	case WM_CREATE:
		// Preprocess arguments
		if (lstrcmpW(szCmdLine, L"--startup") == 0) {
			bPortable = false;
		} else if (lstrcmpW(szCmdLine, L"--restore_tray") == 0) {
			bRestoretray = TRUE;
			HKEY key; DWORD dwDisp;
			if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &dwDisp) == ERROR_SUCCESS) {
				DWORD value = 0;
				RegSetValueExW(key, L"HideTrayIcon", 0, REG_DWORD, (const BYTE*) &value, sizeof(value));
				RegCloseKey(key);
			}
			HWND hKakaoTalkAdGuardMain = FindWindow(L"KakaoTalkAdGuard", NULL);
			if (hKakaoTalkAdGuardMain) {
				SendMessage(hKakaoTalkAdGuardMain, WM_RECHECK, NULL, NULL);
			}
			PostQuitMessage(0);
		}

		bClose = CheckMultipleExecution(hInst, hWnd, szWindowClass);
		hTimer = (HANDLE) SetTimer(hWnd, 1, updateRate, (TIMERPROC) TimerProc);
		CheckStartup(hInst, hWnd);
		if (!hideTrayIcon && !bClose && !bRestoretray) {
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
		CheckMenuItem((HMENU)wParam, IDM_STARTONSYSTEMSTARTUP, MF_BYCOMMAND | (autoStartup ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem((HMENU)wParam, IDM_CHECKUPDATE, MF_BYCOMMAND | (bCheckUpdate ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem((HMENU)wParam, IDM_HIDEMAINPANNELAD, MF_BYCOMMAND | (bHideMainPannelAd ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem((HMENU)wParam, IDM_UPDATEBANNERADREGISTRY, MF_BYCOMMAND | (bUpdateBannerAdRegistry ? MF_CHECKED : MF_UNCHECKED));

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDM_HIDETRAYICON:
			HideTrayIcon(hInst, hWnd, nid);
			break;
		case IDM_STARTONSYSTEMSTARTUP:
			ToggleStartup(hWnd);
			break;
		case IDM_CHECKUPDATE:
			ToggleCheckUpdate(hWnd);
			break;
		case IDM_HIDEMAINPANNELAD:
			ToggleHideMainPannelAd(hWnd);
			break;
		case IDM_UPDATEBANNERADREGISTRY:
			ToggleUpdateBannerAdRegistry(hWnd);
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
	// Check autoruns
	HKEY key; DWORD dwDisp;
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_RUN, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);
	autoStartup = (RegQueryValueExW(key, L"KakaoTalkAdGuard", 0, NULL, 0, NULL) == NO_ERROR);
	RegCloseKey(key);
	SendMessage(hWnd, WM_INITMENU, 0, 0);
	
	// Check HideTrayIcon
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &key, &dwDisp);
	DWORD dwType = REG_DWORD;
	DWORD dwDataSize = sizeof(DWORD);
	DWORD dwValue = 0;
	hideTrayIcon = (RegQueryValueExW(key, L"HideTrayIcon", 0, &dwType, (LPBYTE)&dwValue, &dwDataSize) == ERROR_SUCCESS && dwValue != 0);
	bCheckUpdate = (RegQueryValueExW(key, L"CheckUpdate", 0, &dwType, (LPBYTE)&dwValue, &dwDataSize) == ERROR_SUCCESS && dwValue != 0);
	bHideMainPannelAd = (RegQueryValueExW(key, L"HideMainPannelAd", 0, &dwType, (LPBYTE)&dwValue, &dwDataSize) == ERROR_SUCCESS && dwValue != 0);
	bUpdateBannerAdRegistry = (RegQueryValueExW(key, L"UpdateBannerAdRegistry", 0, &dwType, (LPBYTE)&dwValue, &dwDataSize) == ERROR_SUCCESS && dwValue != 0);

	RegCloseKey(key);

	// Check updates
	if (bCheckUpdate) {
		std::wstring latest = GetLatestReleaseTag(L"loopback-kr", L"KakaoTalkAdGuard");
		if (!latest.empty()) {
			WCHAR buffer[256] = { 0 };
			int len = LoadStringW(GetModuleHandle(NULL), IDS_APP_VERSION, buffer, _countof(buffer));
			std::wstring current = std::wstring(buffer, len);

			if (latest != current) {
				/*NOTIFYICONDATA nid = { sizeof(nid) };
				nid.uFlags = NIF_INFO;
				nid.dwInfoFlags = NIIF_INFO;
				LoadStringW(hInst, IDS_NEWUPDATE_TITLE, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
				LoadStringW(hInst, IDS_NEWUPDATE_CONTENT, nid.szInfo, ARRAYSIZE(nid.szInfo));*/
				std::wstring msg = L"New update released.\n\nCurrent version: " + current + L"\nLatest version: " + latest + L"\n\n"
					L"Do you want to open the release website?";
				int ret = MessageBoxW(hWnd, msg.c_str(), L"KakaoTalk AdGuard",
					MB_YESNO | MB_ICONQUESTION);

				if (ret == IDYES) {
					ShellExecuteW(NULL, L"open", L"https://github.com/loopback-kr/KakaoTalkAdGuard/releases", NULL, NULL, SW_SHOWNORMAL);
				}
			}
		}
	}

	return 0;
}

BOOL ToggleHideMainPannelAd(HWND hWnd) {
	DWORD dwValue = 0; DWORD dwType = REG_DWORD; HKEY key; DWORD dwDisp; BOOL regHideMainPannelAd; DWORD dwDataSize = sizeof(DWORD);
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);
	regHideMainPannelAd = (RegQueryValueExW(key, L"HideMainPannelAd", 0, &dwType, (LPBYTE)&dwValue, &dwDataSize) == ERROR_SUCCESS && dwValue != 0);
	bHideMainPannelAd = not regHideMainPannelAd;
	RegSetValueExW(key, L"HideMainPannelAd", 0, REG_DWORD, (const BYTE*)&bHideMainPannelAd, sizeof(bHideMainPannelAd));
	RegCloseKey(key);
	return 0;
}
BOOL ToggleUpdateBannerAdRegistry(HWND hWnd) {
	DWORD dwValue = 0; DWORD dwType = REG_DWORD; HKEY key; DWORD dwDisp; BOOL regUpdateBannerAdRegistry; DWORD dwDataSize = sizeof(DWORD);
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);
	regUpdateBannerAdRegistry = (RegQueryValueExW(key, L"UpdateBannerAdRegistry", 0, &dwType, (LPBYTE)&dwValue, &dwDataSize) == ERROR_SUCCESS && dwValue != 0);
	bUpdateBannerAdRegistry = not regUpdateBannerAdRegistry;
	RegSetValueExW(key, L"UpdateBannerAdRegistry", 0, REG_DWORD, (const BYTE*)&bUpdateBannerAdRegistry, sizeof(bUpdateBannerAdRegistry));
	RegCloseKey(key);
	return 0;
}

BOOL ToggleCheckUpdate(HWND hWnd) {
	DWORD dwValue = 0; DWORD dwType = REG_DWORD; HKEY key; DWORD dwDisp; BOOL regCheckUpdate; DWORD dwDataSize = sizeof(DWORD);
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_CFG, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);
	regCheckUpdate = (RegQueryValueExW(key, L"CheckUpdate", 0, &dwType, (LPBYTE)&dwValue, &dwDataSize) == ERROR_SUCCESS && dwValue != 0);
	bCheckUpdate = not regCheckUpdate;
	RegSetValueExW(key, L"CheckUpdate", 0, REG_DWORD, (const BYTE*)&bCheckUpdate, sizeof(bCheckUpdate));
	RegCloseKey(key);
	return 0;
}

BOOL ToggleStartup(HWND hWnd) {
	HKEY key; DWORD dwDisp;
	RegCreateKeyEx(HKEY_CURRENT_USER, REG_RUN, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &dwDisp);

	if (RegQueryValueExW(key, L"KakaoTalkAdGuard", 0, NULL, 0, NULL) == NO_ERROR) {
		RegDeleteValueW(key, L"KakaoTalkAdGuard");
		autoStartup = false;
	} else {
		WCHAR szFileName[MAX_PATH];
		WCHAR szFileNameFinal[MAX_PATH] = L"\"";
		GetModuleFileName(NULL, szFileName, MAX_PATH);
		lstrcatW(szFileNameFinal, szFileName);
		lstrcatW(szFileNameFinal, L"\"");
		lstrcatW(szFileNameFinal, L" --startup");
		RegSetValueExW(key, L"KakaoTalkAdGuard", 0, REG_SZ, (LPBYTE) szFileNameFinal, (lstrlenW(szFileNameFinal) + 1) * sizeof(WCHAR));
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
		RegCloseKey(key);
	}
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
	HMENU hMenu;
	WCHAR szAppName[MAX_LOADSTRING];
	WCHAR szVersion[MAX_LOADSTRING];
	WCHAR szFullAppName[MAX_LOADSTRING];
	szFullAppName[0] = L'\0';
	LoadStringW(hInst, IDS_APP_NAME, szAppName, MAX_LOADSTRING);
	LoadStringW(hInst, IDS_APP_VERSION, szVersion, MAX_LOADSTRING);
	wcscpy_s(szFullAppName, szAppName);
	wcscat_s(szFullAppName, L" ");
	wcscat_s(szFullAppName, szVersion);

	if (bPortable) {
		hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_TRAY_CONTEXTMENU_PORTABLE));
		ModifyMenuW(hMenu, ID__APP_TITLE, MF_BYCOMMAND | MF_STRING | MF_DISABLED, ID__APP_TITLE, (LPCWSTR) szFullAppName);
	} else {
		hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_TRAY_CONTEXTMENU));
		ModifyMenuW(hMenu, ID__APP_TITLE, MF_BYCOMMAND | MF_STRING | MF_DISABLED, ID__APP_TITLE, (LPCWSTR)szFullAppName);
	}
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

HWND hKakaoTalkMain;
HWND hAdFit;
RECT RectKakaoTalkMain;
BOOL g_bFoundWebAd = FALSE;

typedef struct {
	LPCWSTR targetClassName;
	BOOL bFoundTarget;
} FIND_DATA;


BOOL CALLBACK FindChromeAndHide(HWND hwndChild, LPARAM lParam) {
	FIND_DATA* pFindClassName = reinterpret_cast<FIND_DATA*>(lParam);
	WCHAR className[256];
	if (GetClassName(hwndChild, className, 256) > 0) {
		if (wcsncmp(className, pFindClassName->targetClassName, 17) == 0) {
			ShowWindow(hwndChild, SW_HIDE);
			g_bFoundWebAd = TRUE;
			return FALSE;
		}
	}
	EnumChildWindows(hwndChild, FindChromeAndHide, lParam);

	return TRUE;
}

BOOL CALLBACK EnumWindowProc(HWND hwnd, LPARAM lParam) {
	FIND_DATA* pFindClassName = reinterpret_cast<FIND_DATA*>(lParam);
	HWND parentHandle = GetParent(hwnd);
	WCHAR className[256];
	WCHAR windowText[256] = L"";
	GetClassName(hwnd, className, 256);
	GetWindowText(hwnd, windowText, 256);

	if (wcscmp(className, pFindClassName->targetClassName) == 0 && wcscmp(windowText, L"") == 0) {
		FIND_DATA targetClassName = { L"Chrome_WidgetWin_", FALSE };
		g_bFoundWebAd = FALSE;
		EnumChildWindows(hwnd, FindChromeAndHide, (LPARAM)&targetClassName);
		if (g_bFoundWebAd) {
			ShowWindow(hwnd, SW_HIDE);
		}
	}
	return TRUE;
}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
	HWND parentHandle = GetParent(hwnd);
	WCHAR className[256] = L"";
	WCHAR parentClassName[256] = L"";
	WCHAR windowText[256] = L"";
	GetClassName(hwnd, className, 256);
	GetWindowText(hwnd, windowText, 256);
	RECT Recthwnd;

	if (wcscmp(className, L"EVA_ChildWindow") == 0) {
		if (wcsncmp(windowText, L"OnlineMainView_", 15) == 0) { // Expand chat widget to empty space
			SetWindowPos(hwnd, HWND_TOP, 0, 0, (RectKakaoTalkMain.right - RectKakaoTalkMain.left), (RectKakaoTalkMain.bottom - RectKakaoTalkMain.top - 32), SWP_NOMOVE);
		}
		return TRUE;
	}
	if (wcsncmp(windowText, L"LockModeView_", 13) == 0) { // Expand numpad in Lockdown mode
		HWND hLockdownNumpad = FindWindowEx(hwnd, NULL, L"EVA_ChildWindow", L"");
		if (hLockdownNumpad != NULL)
			SetWindowPos(hwnd, HWND_TOP, 0, 0, (RectKakaoTalkMain.right - RectKakaoTalkMain.left), (RectKakaoTalkMain.bottom - RectKakaoTalkMain.top), SWP_NOMOVE);
	}
	InvalidateRect(parentHandle, NULL, TRUE);

	return TRUE;
}

void SetLUDForAllSubkeys() {
	HKEY hParentKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Kakao\\AdFit", 0, KEY_READ | KEY_WRITE | KEY_ENUMERATE_SUB_KEYS, &hParentKey) != ERROR_SUCCESS) {
		return;
	}

	DWORD dwIndex = 0;
	wchar_t subkeyName[256];
	DWORD subkeyNameSize = sizeof(subkeyName) / sizeof(wchar_t);
	FILETIME ftLastWritten;

	while (RegEnumKeyExW(hParentKey, dwIndex, subkeyName, &subkeyNameSize, NULL, NULL, NULL, &ftLastWritten) == ERROR_SUCCESS) {

		std::wstring subkeyPath = L"SOFTWARE\\Kakao\\AdFit\\";
		subkeyPath += subkeyName;

		HKEY hSubKey;
		DWORD dwDisp;
		if (RegCreateKeyExW(HKEY_CURRENT_USER, subkeyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSubKey, &dwDisp) == ERROR_SUCCESS) {

			auto now = std::chrono::system_clock::now();
			auto posix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
			std::wstring str_time = std::to_wstring(posix_time);

			RegSetValueExW(hSubKey, L"LUD", 0, REG_SZ, (const BYTE*)str_time.c_str(), (str_time.length() + 1) * sizeof(wchar_t));
			RegCloseKey(hSubKey);
		}

		dwIndex++;
		subkeyNameSize = sizeof(subkeyName) / sizeof(wchar_t);
	}
	RegCloseKey(hParentKey);
}

std::wstring GetLatestReleaseTag(const std::wstring& owner, const std::wstring& repo) {
	HINTERNET hSession = WinHttpOpen(L"KakaoTalkAdGuard/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0);
	if (!hSession) return L"";

	HINTERNET hConnect = WinHttpConnect(hSession, L"api.github.com",
		INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) {
		WinHttpCloseHandle(hSession);
		return L"";
	}

	std::wstring path = L"/repos/" + owner + L"/" + repo + L"/releases/latest";
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
		NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		WINHTTP_FLAG_SECURE);
	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return L"";
	}

	const wchar_t* userAgentHeader = L"User-Agent: Win32App";
	WinHttpAddRequestHeaders(hRequest, userAgentHeader, -1, WINHTTP_ADDREQ_FLAG_ADD);

	BOOL bResults = WinHttpSendRequest(hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0,
		0, 0);

	if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

	std::wstring response;
	if (bResults) {
		DWORD dwSize = 0;
		do {
			DWORD dwDownloaded = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;

			if (dwSize == 0) break;

			char* buffer = new char[dwSize + 1];
			ZeroMemory(buffer, dwSize + 1);

			if (WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded)) {
				std::wstring wbuf;
				int len = MultiByteToWideChar(CP_UTF8, 0, buffer, dwDownloaded, NULL, 0);
				if (len > 0) {
					wbuf.resize(len);
					MultiByteToWideChar(CP_UTF8, 0, buffer, dwDownloaded, &wbuf[0], len);
					response += wbuf;
				}
			}
			delete[] buffer;
		} while (dwSize > 0);
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	std::wstring tag = L"";
	size_t pos = response.find(L"\"tag_name\"");
	if (pos != std::wstring::npos) {
		size_t start = response.find(L"\"", pos + 10);
		size_t end = response.find(L"\"", start + 1);
		if (start != std::wstring::npos && end != std::wstring::npos) {
			tag = response.substr(start + 1, end - start - 1);
		}
	}

	return tag;
}

VOID CALLBACK TimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTimer) {
	switch (idEvent) {
	case 1: // Remove KakaoTalk ADs
		// Find main handle
		const WCHAR* kakaoTalkNames[] = {L"카카오톡", L"カカオトーク", L"KakaoTalk"};
		int numNames = sizeof(kakaoTalkNames) / sizeof(kakaoTalkNames[0]);
		for (int i = 0; i < numNames; ++i) {
			hKakaoTalkMain = FindWindow(L"EVA_Window_Dblclk", kakaoTalkNames[i]);
			if (hKakaoTalkMain != NULL)
				break;
		}

		// Block popup AD
		if (bUpdateBannerAdRegistry == 1) {
			SetLUDForAllSubkeys();
		}

		// Scan ADs recursive
		HWND hParent = GetParent(hwnd);
		if (bHideMainPannelAd) {
			GetWindowRect(hKakaoTalkMain, &RectKakaoTalkMain);
			FIND_DATA targetClassName = { L"EVA_Window_Dblclk", FALSE };
			EnumWindows(EnumWindowProc, (LPARAM)&targetClassName);
			EnumChildWindows(hKakaoTalkMain, EnumChildProc, NULL);
		}

		break;
	}
}