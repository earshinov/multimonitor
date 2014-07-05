#include <iostream>
#include <string>
#include <tchar.h>
#include <windows.h>

#include "AnimationSuppressor.h"
#include "utils.h"

std::wstring activeMonitorName;

bool getMonitorName(HWND window, std::wstring & deviceName)
{
	HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
	if (monitor == NULL)
	{
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

BOOL WINAPI handleWindow(HWND window, LPARAM lparam)
{
	std::wstring monitorName;
	if (getMonitorName(window, monitorName) && monitorName == activeMonitorName)
	{
		unsigned long styles = GetWindowLong(window, GWL_STYLE);
		if (styles == 0)
		{
			winapiError(L"GetWindowLong(GWL_STYLE) failed");
			return true;
		}

		unsigned long exStyles = GetWindowLong(window, GWL_EXSTYLE);
		if (exStyles == 0)
		{
			winapiError(L"GetWindowLong(GWL_EXSTYLE) failed");
			return true;
		}
		
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

	return true;
}

int _tmain(int argc, _TCHAR* argv[])
{
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

