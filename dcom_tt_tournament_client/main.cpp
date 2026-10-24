#include <windows.h>
#include "TournamentEvents.h"
#include "Utils.h"
#include <objbase.h>
#include <iostream>
#include <thread>

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

void HandleJudgeCommands(ITournamentManager* pManager) {
    std::wcout << L"You've entered tt tournament management system as judge.\n"
        L"You can report match results with command: [player1] [player2] [score1] [score2]\n" 
        L"Use command 'send_res' to report results. Use command 'exit' to exit.\n"
        << std::endl;

    bool waiting = true;
    std::string player1, player2, command;
    int score1, score2;

    while (waiting) {
        std::cin >> command;
        if (command == "exit") {
            ::SetEvent(exitEvent);
            return;
        } else if (command == "send_res") {
            std::cin >> player1 >> player2 >> score1 >> score2;
            BSTR p1_bstr = WStringToBSTR(str_to_wstring(player1));
            BSTR p2_bstr = WStringToBSTR(str_to_wstring(player2));

            HRESULT hr = pManager->SaveMatchResults(p1_bstr, p2_bstr, score1, score2);
            if (FAILED(hr)) {
                std::cerr << "Match results report failed. ERROR: 0x" << std::hex << hr << std::endl;
            }

            ::SysFreeString(p1_bstr);
            ::SysFreeString(p2_bstr);
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
        }
    }
}

void HandleAdminCommands(ITournamentManager* pManager) {
    std::wcout << L"You've entered tt tournament management system as admin.\n"
        L"Available commands:\n\t'get_stats': [player_name]\n\t'reg_player': [player_name] [rating]"
        L"\n\t'start_tournament': [tables].\n\t'exit'\n"
        << std::endl;

    bool waiting = true;
    std::string player, command;
    PlayerStats stats{};
    HRESULT hr;

    while (waiting) {
        std::cin >> command;
        if (command == "exit") {
            ::SetEvent(exitEvent);
            return;

        } else if (command == "get_stats") {
            std::cin >> player;
            BSTR p_bstr = WStringToBSTR(str_to_wstring(player));

            hr = pManager->GetPlayerStats(p_bstr, &stats);
            if (FAILED(hr)) {
                std::cerr << "Failed to get player stats. ERROR: 0x" << std::hex << hr << std::endl;
            }

            ::SysFreeString(p_bstr);

        } else if (command == "reg_player") {
            int rating;
            std::cin >> player >> rating;
            BSTR p_bstr = WStringToBSTR(str_to_wstring(player));

            hr = pManager->RegisterPlayer(p_bstr, rating);
            if (FAILED(hr)) {
                std::cerr << "Failed to register player. ERROR: 0x" << std::hex << hr << std::endl;
            }

            ::SysFreeString(p_bstr);
        } else if (command == "start_tournament") {
            hr = pManager->StartTournament();
            if (FAILED(hr)) {
                std::cerr << "Failed to start tournament. ERROR: 0x" << std::hex << hr << std::endl;
            }
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
        }
    }
}

void ClientActionsHandler(IGlobalInterfaceTable* pGIT, DWORD dwManagerId, Role role) {
    ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    ITournamentManager* pManager = nullptr;
    HRESULT hr = pGIT->GetInterfaceFromGlobal(dwManagerId, IID_ITournamentManager, (void**)&pManager);

    if (SUCCEEDED(hr)) {
        switch (role) {
        case ROLE_JUDGE:
            HandleJudgeCommands(pManager);
            break;
        case ROLE_ADMIN:
            HandleAdminCommands(pManager);
            break;
        default:
            std::cerr << "Unknown role: " << role << std::endl;
        }
    } else {
        std::cerr << "Failed to get interface from GIT. ERROR: 0x" << std::hex << hr << std::endl;
    }
    
    ::CoUninitialize();
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

    /*if (!RegisterTLB(L"..\\..\\dcom_tt_tournament_manager\\tournament_server.tlb")) {
        ::CoUninitialize();
        return 1;
    }*/

    COSERVERINFO server_info = {};
    server_info.pwszName = nullptr; // localhost

    MULTI_QI mqi = {};
    mqi.pIID = &IID_ITournamentManager;
    mqi.pItf = nullptr;

    std::wcout << L"Nuka Nuka" << std::endl;

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

    IGlobalInterfaceTable* pGIT = nullptr;
    hr = ::CoCreateInstance(
        CLSID_StdGlobalInterfaceTable,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IGlobalInterfaceTable,
        (void**)&pGIT
    );

    DWORD dwManagerId = 0;
    hr = pGIT->RegisterInterfaceInGlobal(pManager, IID_ITournamentManager, &dwManagerId);
    std::thread client_io(ClientActionsHandler, pGIT, dwManagerId, role);

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
    client_io.join();
    std::wcout << L"\nShutting down the client..." << std::endl;
    pManager->Unsubscribe(static_cast<ITournamentEvents*>(server_events));

    server_events->Release();
    pManager->Release(); 

    ::CoUninitialize();
    return 0;
}