#include <iostream>
#include <windows.h>

#include "utils.h"

void winapiError(const std::wstring & message)
{
	DWORD code = GetLastError();
	LPWSTR winapiMessage = new WCHAR[1024];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, LANG_NEUTRAL, winapiMessage, 1024, NULL);
	std::wcerr << message << L" (code " << code << L"): " << winapiMessage << std::endl;
}