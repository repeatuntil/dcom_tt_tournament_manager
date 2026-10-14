#include "Utils.h"
#include <oleauto.h>
#include <iostream>

std::wstring BSTRToWstring(BSTR bstr) {
	if (!bstr) {
		return std::wstring();
	}
	return std::wstring(bstr, ::SysStringLen(bstr));
}

BSTR WStringToBSTR(const std::wstring& ws) {
	return ::SysAllocStringLen(ws.data(), static_cast<UINT>(ws.length()));
}

std::wstring str_to_wstring(const std::string& str) {
    if (str.empty()) {
        return L"";
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring res(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &res[0], size_needed);

    return res;
}

bool RegisterTLB(const wchar_t* filepath) {
    WCHAR absPath[MAX_PATH];
    DWORD dwRet = ::GetFullPathNameW(filepath, MAX_PATH, absPath, nullptr);
    if (dwRet == 0) {
        std::cerr << "Failed to resolve absolute .tlb path." << std::endl;
        return false;
    }

    ITypeLib* pTypeLib = nullptr;
    HRESULT hr = ::LoadTypeLib(absPath, &pTypeLib);

    if (SUCCEEDED(hr)) {
        hr = ::RegisterTypeLib(pTypeLib, absPath, nullptr);
        pTypeLib->Release();

        if (FAILED(hr)) {
            std::cerr << "Failed to register type lib : ERROR 0x" << std::hex << hr << std::endl;
            return false;
        }

        return true;
    }

    std::cerr << "Failed to load type lib : ERROR 0x" << std::hex << hr << std::endl;
    return false;
}