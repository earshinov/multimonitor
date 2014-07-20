#pragma once

#include <string>
#include <tchar.h>

#ifndef countof
#define countof(a) (sizeof(a)/sizeof(a[0]))
#endif

void winapiError(const std::wstring & message);