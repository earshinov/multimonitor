#include <clocale>
#include <iostream>
#include <string>
#include <tchar.h>
#include <windows.h>

#include "AnimationSuppressor.h"
#include "utils.h"

std::wstring activeMonitorName;
HWND shell;
HWND desktop;

bool getMonitorName(HWND window, std::wstring & deviceName)
{
	HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
	if (monitor == NULL)
	{
		if (GetLastError() != S_OK)
			winapiError(L"MonitorFromWindow failed");
		return false;
	}

	MONITORINFOEX mi;
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(monitor, &mi))
	{
		winapiError(L"GetMonitorInfo failed");
		return false;
	}

	deviceName = mi.szDevice;
	return true;
}


bool program();
BOOL WINAPI handleWindow(HWND window, LPARAM lparam);
void focusDesktop();
bool getDesktopWindows();
LRESULT CALLBACK dummyWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
void passFocus();
void _debugFocus(const std::wstring & token);

#define S(x) L#x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define debugFocus() _debugFocus(S__LINE__)


int _tmain(int argc, _TCHAR * argv[])
{
	std::setlocale(LC_ALL, "");

	if (argc != 2 || _tcscmp(argv[1], L"minimize") != 0)
	{
		std::wcerr
			<< L"Usage: multimonitor.exe [COMMAND]" << std::endl
			<< std::endl
			<< L"Available commands:" << std::endl
			<< L"  minimize - minimize windows on the current screen" << std::endl;
		return 1;
	}

	return program() ? 0 : 1;
}


bool program()
{
	HWND window = GetForegroundWindow();
	if (window == NULL)
	{
		winapiError(L"GetForegroundWindow failed");
		return false;
	}

	if (!getMonitorName(window, activeMonitorName))
		return false;
	
	AnimationSuppressor _;
	if (!EnumWindows(handleWindow, 0))
	{
		winapiError(L"EnumWindows failed");
		return false;
	}

	focusDesktop();

	return true;
}

BOOL WINAPI handleWindow(HWND window, LPARAM lparam)
{
	std::wstring monitorName;
	if (getMonitorName(window, monitorName) && monitorName == activeMonitorName)
	{
		unsigned long styles = GetWindowLong(window, GWL_STYLE);
		unsigned long exStyles = GetWindowLong(window, GWL_EXSTYLE);

		if ((styles & WS_VISIBLE) == WS_VISIBLE && (styles & WS_POPUP) == 0 && (exStyles & WS_EX_TOOLWINDOW) == 0)
		{
			// To minimize a window we can use ShowWindow or SetWindowPlacement.
			// SetWindowPlacement does not trigger animation (which is a plus), but mangles window position,
			// especially if it is docked to a side of the screen (Win+arrow hotkeys in Windows 7).
			// ShowWindow function triggers animation and we have to temporarily
			// disable it system-wide (see AnimationSuppressor class)
			if (!ShowWindow(window, SW_MINIMIZE))
			{
				winapiError(L"SetWindowPlacement failed");
				return true;
			}
		}
	}
	return true;
}

// Try to focus desktop window.
//
// Unfortunately, to accomplish this, we need to create our own dummy window, focus it and only then focus desktop window.
// This is due to restrictions imposed by Windows - it does not allow us to SetForegroundWindow unless current foreground
// window belongs to our process (there are other options, but they are not to applicable -- see SetForegroundWindow in MSDN).
//
// There is also a hack with AttachThreadInput, see links below.
//
// - http://stackoverflow.com/questions/3772233/win32-setforegroundwindow-unreliable
// - http://stackoverflow.com/questions/23715026/allow-background-application-to-set-foreground-window-of-different-process
//
void focusDesktop()
{
	if (!getDesktopWindows())
		return;

	if (!AttachThreadInput(GetWindowThreadProcessId(shell, NULL), GetCurrentThreadId(), true))
	{
		winapiError(L"AttachThreadInput failed");
		return;
	}

	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.style = 0;
	wc.lpfnWndProc = dummyWindowProc;
	wc.lpszClassName = L"dummy";
	RegisterClassEx(&wc);

	HWND dummy = CreateWindow(L"dummy", L"", 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, NULL, NULL);
	if (dummy == NULL)
	{
		winapiError(L"CreateWindow failed");
		return;
	}

	ShowWindow(dummy, SW_SHOWMINIMIZED);
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

bool getDesktopWindows()
{
	HWND shell = GetShellWindow();
	if (shell == NULL)
	{
		winapiError(L"GetShellWindow failed");
		return false;
	}

	HWND desktop = shell;
	desktop = FindWindowEx(desktop, NULL, L"SHELLDLL_DefView", NULL);
	if (desktop == NULL)
	{
		winapiError(L"FindWindow(1) failed");
		return false;
	}

	desktop = FindWindowEx(desktop, NULL, L"SysListView32", L"FolderView");
	if (desktop == NULL)
	{
		winapiError(L"FindWindow(2) failed");
		return false;
	}

	::shell = shell;
	::desktop = desktop;
	return true;
}

LRESULT CALLBACK dummyWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	const int TIMER_ID = 1;

	switch (message)
	{
	case WM_CREATE:
		SetTimer(window, TIMER_ID, 50, NULL);
		break;
	case WM_TIMER:
		if (wParam == TIMER_ID)
		{
			KillTimer(window, TIMER_ID);
			ShowWindow(window, SW_SHOWNORMAL);
			DestroyWindow(window);
			break;
		}
		break;
	case WM_DESTROY:
		passFocus();
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(window, message, wParam, lParam);
}

void passFocus()
{
	debugFocus();

	if (!SetForegroundWindow(shell))
	{
		winapiError(L"SetForegroundWindow failed");
		return;
	}

	debugFocus();

	if (!SetFocus(desktop))
	{
		winapiError(L"SetFocus failed");
		return;
	}

	debugFocus();
}

void _debugFocus(const std::wstring & token)
{
	std::wcerr << L"--- " << token << std::endl;
	std::wcerr << std::hex << GetForegroundWindow() << std::endl;
	std::wcerr << std::hex << GetActiveWindow() << std::endl;
	std::wcerr << std::hex << GetFocus() << std::endl;
}

