#include <iostream>
#include <windows.h>

#include "utils.h"

void winapiError(const std::wstring & message)
{
	wchar_t buffer[1024];

	DWORD code = GetLastError();

	LPWSTR winapiMessage = buffer;
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, LANG_NEUTRAL, winapiMessage, countof(buffer), NULL) == 0)
		winapiMessage = L"<COUND NOT FORMAT MESSAGE>";
	
	std::wcerr << message << L" (code " << code << L"): " << winapiMessage << std::endl;
}	