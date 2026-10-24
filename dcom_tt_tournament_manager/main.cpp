#include "ClassFactory.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>

int main() {
	HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    DWORD dwFactID;

	if (FAILED(hr)) {
		std::cerr << "Failed to init COM. ERROR 0x" << std::hex << hr << std::endl;
		return 1;
	}

    hr = ::CoInitializeSecurity(
        NULL,                        
        -1,                          
        NULL,                        
        NULL,                        
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, 
        NULL,                        
        EOAC_NONE,                   
        NULL                         
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to init COM Security. ERROR: 0x" << std::hex << hr << std::endl;
        ::CoUninitialize();
        return 1;
    }

    /*if (!RegisterTLB(L"..\\..\\dcom_tt_tournament_manager\\tournament_server.tlb")) {
        ::CoUninitialize();
        return 1;
    }*/

    auto pTournamentFactory = new TournamentClassFactory();
    hr = ::CoRegisterClassObject(
        CLSID_TournamentManager,
        static_cast<IClassFactory*>(pTournamentFactory),
        CLSCTX_LOCAL_SERVER, 
        REGCLS_MULTIPLEUSE,
        &dwFactID
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to register class object. ERROR: 0x" << std::hex << hr << std::endl;
        delete pTournamentFactory;
        ::CoUninitialize();
        return 1;
    }

    std::cout << "Successfully started outer process COM server!" << std::endl;
    MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    
    ::CoRevokeClassObject(dwFactID);
    delete pTournamentFactory;
    ::CoUninitialize();
    std::cout << "COM server stopped working. All resources released." << std::endl;
    return 0;
}