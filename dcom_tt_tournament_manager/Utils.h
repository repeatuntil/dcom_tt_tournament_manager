#pragma once
#include "windows.h"
#include <string>

// string converters
std::wstring BSTRToWstring(BSTR bstr);
BSTR WStringToBSTR(const std::wstring& ws);
std::wstring str_to_wstring(const std::string& str);

// autoreg for tlb
bool RegisterTLB(const wchar_t* filenpath);