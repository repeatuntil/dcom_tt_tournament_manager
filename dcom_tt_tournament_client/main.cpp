#include <windows.h>
#include "TournamentEvents.h"
#include "Utils.h"
#include <iostream>

HANDLE exitEvent = ::CreateEvent(nullptr, FALSE, FALSE, NULL);

BOOL WINAPI ConsoleExitHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        if (exitEvent != NULL) {
            ::SetEvent(exitEvent);
        }

        return TRUE;
    }
    return FALSE;
}

bool TryAuthorizeAsAdmin(ITournamentManager* pManager, Role& role) {
    std::string password;
    HRESULT hr;
    int i;

    // 5 tries
    for (i = 0; i < 5; i++) {
        std::wcout << L"Enter password: ";
        std::cin >> password;
        BSTR bstr_pass = WStringToBSTR(str_to_wstring(password));
        int success;

        hr = pManager->Authorize(bstr_pass, &success);
        ::SysFreeString(bstr_pass);

        if (FAILED(hr)) {
            std::cerr << "Authentication failed. ERROR: 0x" << std::hex << hr << std::endl;
            return false;
        }

        if (success) {
            break;
        }

        std::wcout << L"\nWrong password. Try again" << std::endl;
    }

    if (i >= 5) {
        return false;
    }

    role = ROLE_ADMIN;
    return true;
}

int main() {
	HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		std::cerr << "Failed to init COM Client. ERROR 0x" << std::hex << hr << std::endl;
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
        std::cerr << "Failed to init COM Security (for client). ERROR: 0x" << std::hex << hr << std::endl;
        ::CoUninitialize();
        return 1;
    }

    if (!RegisterTLB(L"..\\..\\dcom_tt_tournament_manager\\tournament_server.tlb")) {
        ::CoUninitialize();
        return 1;
    }

    COSERVERINFO server_info = {};
    server_info.pwszName = nullptr; // localhost

    MULTI_QI mqi = {};
    mqi.pIID = &IID_ITournamentManager;
    mqi.pItf = nullptr;

    ::CoCreateInstanceEx(
        CLSID_TournamentManager,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        &server_info,
        1,
        &mqi
    );

    if (FAILED(mqi.hr)) {
        std::cerr << "Failed to connect to tournament server. ERROR: 0x" << std::hex << mqi.hr << std::endl;
        ::CoUninitialize();

        int wait;
        std::cin >> wait;
        return 1;
    }

    std::wcout << L"Successfully connected to tournament server" << std::endl;
    auto pManager = static_cast<ITournamentManager*>(mqi.pItf);
    
    int table = -1;
    Role role = ROLE_JUDGE;
    std::string is_admin_ans;
    bool admin_confirmed = false;

    std::wcout << L"Are you connecting as admin (yes/no)? ";
    std::cin >> is_admin_ans;

    if (is_admin_ans == "yes") {
        admin_confirmed = TryAuthorizeAsAdmin(pManager, role);
        if (!admin_confirmed) {
            std::cerr << "Authentication failed. Number of attempts exceeded" << std::endl;
            pManager->Release();
            ::CoUninitialize();
            return 1;
        }

    } else {
        std::wcout << L"Input number of table: ";
        std::cin >> table;
    }

    // for ctrl + c
    ::SetConsoleCtrlHandler(ConsoleExitHandler, TRUE);
    auto server_events = new TournamentEvents(exitEvent);

    hr = pManager->Subscribe(server_events, role, table);
    if (FAILED(hr)) {
        std::cerr << "Failed to subscribe to updates. ERROR: 0x" << std::hex << hr << std::endl;
        server_events->Release();
        pManager->Release();
        ::CoUninitialize();
        return 1;
    }

    MSG msg;
    DWORD nCount = 1;

    while (true) {
        DWORD result = ::MsgWaitForMultipleObjects(nCount, &exitEvent, FALSE, INFINITE, QS_ALLINPUT);

        if (result == WAIT_OBJECT_0) {
            std::wcout << L"Server send exit event or client was closed forcibly" << std::endl;
            break;
        }

        if (result == WAIT_OBJECT_0 + nCount) {
            while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }
    }

    // clear all and exit
    std::wcout << L"\nShutting down the client..." << std::endl;
    pManager->Unsubscribe(static_cast<ITournamentEvents*>(server_events));

    server_events->Release();
    pManager->Release(); 

    ::CoUninitialize();
    return 0;
}