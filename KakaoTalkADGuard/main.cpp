#include "framework.h"
#include "KakaoTalkADGuard.h"

#include <bcrypt.h>
#include <tchar.h>
#include <TlHelp32.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <deque>
#include <list>
#include <thread>

#pragma comment(lib, "ntdll")
#pragma commnet(lib, "user32")

#define EXE_NAME L"KaKaoTalk.exe"
#define SEMAPHORE_STRING L"97C4DDD9"

#define X64_PATH L"C:\\Program Files (x86)\\Kakao\\KakaoTalk\\KaKaoTalk.exe"
#define X86_PATH L"C:\\Program Files\\Kakao\\KakaoTalk\\KaKaoTalk.exe"

#define MESSAGE_TITLE L"KaKaoMulti"

#define NT_SUCCESS(status) (status >= 0)
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004L

typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO {
	HANDLE HandleValue;
	ULONG_PTR HandleCount;
	ULONG_PTR PointerCount;
	ULONG GrantedAccess;
	ULONG ObjectTypeIndex;
	ULONG HandleAttributes;
	ULONG Reserved;
} PROCESS_HANDLE_TABLE_ENTRY_INFO;

typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION {
	ULONG_PTR NumberOfHandles;
	ULONG_PTR Reserved;
	PROCESS_HANDLE_TABLE_ENTRY_INFO Handles[1];
} PROCESS_HANDLE_SNAPSHOT_INFORMATION;

enum PROCESSINFOCLASS {
	ProcessHandleInformation = 51
};

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation = 0,
	ObjectNameInformation = 1,
	ObjectTypeInformation = 2,
	ObjectAllTypesInformation = 3,
	ObjectHandleInformation = 4
} OBJECT_INFORMATION_CLASS;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING;

typedef struct _OBJECT_NAME_INFORMATION {
	UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION;

extern "C" NTSTATUS __stdcall NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
extern "C" NTSTATUS __stdcall NtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
extern "C" int __stdcall MessageBoxTimeoutW(HWND hwnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId, DWORD dwMilliseconds);








// Global variables
HINSTANCE       hInst;
LPWSTR          szCmdLine = 0;
WCHAR           szTitle[MAX_LOADSTRING];
WCHAR           szWindowClass[MAX_LOADSTRING];
UINT            updateRate = 100;
BOOL            autoStartup = false;
BOOL            hideTrayIcon = false;
NOTIFYICONDATA  nid = {sizeof(nid)};
BOOL            bPortable = true;

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
			}
			RegCloseKey(key);
			HWND hKakaoTalkADGuardMain = FindWindow(L"KakaoTalkADGuard", NULL);
			if (hKakaoTalkADGuardMain) {
				SendMessage(hKakaoTalkADGuardMain, WM_RECHECK, NULL, NULL);
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
		WCHAR szFileNameFinal[MAX_PATH] = L"\"";
		GetModuleFileName(NULL, szFileName, MAX_PATH);
		lstrcatW(szFileNameFinal, szFileName);
		lstrcatW(szFileNameFinal, L"\"");
		lstrcatW(szFileNameFinal, L" --startup");
		RegSetValueExW(key, L"KakaoTalkADGuard", 0, REG_SZ, (LPBYTE) szFileNameFinal, (lstrlenW(szFileNameFinal) + 1) * sizeof(WCHAR));
		autoStartup = true;
	}
	RegCloseKey(key);
	return 0;
}

BOOL HideTrayIcon(HINSTANCE hInst, HWND hWnd, NOTIFYICONDATA nid) {
	WCHAR msgboxHideTray[MAX_LOADSTRING];
	LoadStringW(hInst, IDS_MSGBOX_HIDETRAY, msgboxHideTray, MAX_LOADSTRING);
	MessageBox(hWnd, msgboxHideTray, L"KakaoTalk ADGuard", MB_ICONINFORMATION);
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
		MessageBeep(MB_ICONASTERISK);
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
	HMENU hMenu;
	if (bPortable) {
		hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_TRAY_CONTEXTMENU_PORTABLE));
	} else {
		hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_TRAY_CONTEXTMENU));
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

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lparam) {
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if ((DWORD) lparam == pid) {
		return FALSE; // found
	} else {
		return TRUE; // continue
	}
}

RECT RectKakaoTalkMain;
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
	HWND parentHandle = GetParent(hwnd);
	WCHAR className[256] = L"";
	WCHAR windowText[256] = L"";
	GetClassName(hwnd, className, 256);
	GetWindowText(hwnd, windowText, 256);

	if (wcscmp(className, L"EVA_ChildWindow") == 0) {
		if (wcsncmp(windowText, L"OnlineMainView_", 15) == 0) { // Expand chat widget to empty space
			SetWindowPos(hwnd, HWND_TOP, 0, 0, (RectKakaoTalkMain.right - RectKakaoTalkMain.left), (RectKakaoTalkMain.bottom - RectKakaoTalkMain.top - 32), SWP_NOMOVE);
		}
		return TRUE;
	}
	if (wcscmp(className, L"BannerAdWnd") == 0) {
		ShowWindow(hwnd, SW_HIDE);
		return TRUE;
	}
	if (wcscmp(className, L"RichPopWnd") == 0) {
		ShowWindow(hwnd, SW_HIDE);
		return TRUE;
	}
	if (wcscmp(className, L"EVA_VH_ListControl_Dblclk") == 0) {
		InvalidateRect(hwnd, NULL, TRUE);
		return TRUE;
	}
	return TRUE;
}

VOID CALLBACK TimerProc(HWND hwnd, UINT message, UINT idEvent, DWORD dwTimer) {
	switch (idEvent) {
	case 1: // Remove KakaoTalk ADs
		// Find main handle
		HWND hKakaoTalkMain = FindWindow(L"EVA_Window_Dblclk", L"카카오톡");

		// Block banner AD
		HWND hKakaoTalkAd = FindWindow(L"EVA_Window_Dblclk", L"");
		RECT RectKakaoTalkAd;
		if (GetParent(hKakaoTalkAd) == hKakaoTalkMain) {
			GetWindowRect(hKakaoTalkAd, &RectKakaoTalkAd);
			int height = RectKakaoTalkAd.bottom - RectKakaoTalkAd.top;
			if (height == 100) {
				ShowWindow(hKakaoTalkAd, SW_HIDE);
			}
		}

		// Scan ADs recursive
		GetWindowRect(hKakaoTalkMain, &RectKakaoTalkMain);
		EnumChildWindows(hKakaoTalkMain, EnumChildProc, NULL);
		EnumChildWindows(hKakaoTalkAd, EnumChildProc, NULL);
		
		// Sanity check for Popup AD
		DWORD pid_main = 0;
		DWORD pid_popup = 0;
		HWND hPopupWnd = FindWindow(L"RichPopWnd", L"");
		GetWindowThreadProcessId(hKakaoTalkMain, &pid_main);
		GetWindowThreadProcessId(hPopupWnd, &pid_popup);
		if (pid_main == pid_popup)
			ShowWindow(hPopupWnd, SW_HIDE);
		break;
	}
}

int launchNewKakaoTalkint() {
	// Find running process, change information
	std::vector<DWORD> pids = FindProcesses(EXE_NAME);
	for (DWORD pid : pids) {
		HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE + PROCESS_QUERY_INFORMATION, FALSE, pid);
		if (!hProcess) continue;

		// Copy processinformation to buffer
		DWORD dwSize = 0;
		NTSTATUS status = STATUS_INFO_LENGTH_MISMATCH;
		std::unique_ptr<BYTE[]> handleInfoBuffer;
		while (status == STATUS_INFO_LENGTH_MISMATCH) {
			dwSize += 1024;
			handleInfoBuffer = std::make_unique<BYTE[]>(dwSize);
			/*ProcessHandleInformation: Support Windows NT 6.2 or newer*/
			status = NtQueryInformationProcess(hProcess, ProcessHandleInformation, handleInfoBuffer.get(), dwSize, NULL);
			if (NT_SUCCESS(status))
				break;
		}
		PROCESS_HANDLE_SNAPSHOT_INFORMATION* info = (PROCESS_HANDLE_SNAPSHOT_INFORMATION*) handleInfoBuffer.get();

		for (auto i = 0; i < info->NumberOfHandles; i++) {
			HANDLE hTarget;
			if (!DuplicateHandle(hProcess, info->Handles[i].HandleValue, GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
				continue;

			BYTE pwName[1024] = {0,};
			BYTE pwType[1024] = {0,};
			if (!NT_SUCCESS(NtQueryObject(hTarget, ObjectNameInformation, pwName, sizeof(pwName), nullptr)) ||
				!NT_SUCCESS(NtQueryObject(hTarget, ObjectTypeInformation, pwType, sizeof(pwType), nullptr)))
				continue;
			CloseHandle(hTarget);

			UNICODE_STRING* name = (UNICODE_STRING*) pwName;
			UNICODE_STRING* type = (UNICODE_STRING*) pwType;
			if (name->Buffer == nullptr || type->Buffer == nullptr)
				continue;

			if (_wcsicmp(type->Buffer, L"Semaphore") != 0)
				continue;

			if (wcsstr(name->Buffer, SEMAPHORE_STRING) == NULL)
				continue;

			if (!DuplicateHandle(hProcess, info->Handles[i].HandleValue, GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
				continue;

			CloseHandle(hTarget);
		}
	}

	// Run new process
	STARTUPINFO si = {0,};
	PROCESS_INFORMATION pi = {0,};

	if (!CreateProcess(X64_PATH, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		MessageBoxTimeoutW(NULL, L"Load fail.", MESSAGE_TITLE, MB_OK | MB_ICONINFORMATION, 0, 4000);
		return 0;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	MessageBoxTimeoutW(NULL, L"Load Success. Wait for seconds...", MESSAGE_TITLE, MB_OK | MB_ICONINFORMATION, 0, 4000);
	return 0;
}

std::vector<DWORD> FindProcesses(const WCHAR* pwFileName) {
	std::vector<DWORD> pids;
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return pids;
	}

	PROCESSENTRY32 pe = {0,};
	pe.dwSize = sizeof(pe);

	Process32First(hSnapshot, &pe);
	while (Process32Next(hSnapshot, &pe)) {
		if (_wcsicmp(pe.szExeFile, pwFileName) != 0)
			continue;
		pids.push_back(pe.th32ProcessID);
	}

	if (hSnapshot)
		::CloseHandle(hSnapshot);

	return pids;
}