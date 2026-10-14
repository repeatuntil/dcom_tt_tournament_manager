#include "TournamentManager.h"
#include "Utils.h"
#include <algorithm>
#include <intrin.h>
#include <guiddef.h>
#include <mutex>
#include <windows.h>
#include <iostream>

const std::wstring PASSWORD = L"i_am_admin";

Player* Match::GetWinner() {
	if (!finished)
		return nullptr;

	return score1 > score2 ? player1 : player2;
}

TournamentManager::TournamentManager() : ref_count(1) {
	obj_count++;
}

TournamentManager::~TournamentManager() {
	std::wcout << "TournamentManager deleted. Ref count = " << ref_count << std::endl;

	for (const auto p : players) {
		delete p;
	}
	players.clear();

	for (const auto& conn : conn_list) {
		if (conn.client_sink)
			conn.client_sink->Release();
	}

	if (--obj_count == 0 && TournamentClassFactory::locks_count == 0) {
		::PostQuitMessage(0);
	}
}

STDMETHODIMP_(ULONG) TournamentManager::AddRef() {
	return ++ref_count;
}

STDMETHODIMP_(ULONG) TournamentManager::Release() {
	ULONG rem_ref = --ref_count;
	std::wcout << L"release remain refs=" << rem_ref << std::endl;
	if (rem_ref == 0) {
		delete this;
	}
	return rem_ref;
}

STDMETHODIMP TournamentManager::QueryInterface(REFIID riid, void** ppvObject) {
	wchar_t szGuid[64];
	StringFromGUID2(riid, szGuid, 64);
	std::wcout << L"QueryInterface requested GUID: " << szGuid << std::endl;

	if (ppvObject == nullptr) {
		return E_POINTER;
	}

	if (riid == IID_IUnknown) {
		*ppvObject = static_cast<IUnknown*>(this);
	} else if (riid == IID_ITournamentManager) {
		std::wcout << "IM HERE!!!" << std::endl;
		*ppvObject = static_cast<ITournamentManager*>(this);
	} else {
		return E_NOINTERFACE;
	}

	AddRef();
	std::wcout << L"ref_count=" << ref_count << std::endl;
	return S_OK;
}

STDMETHODIMP TournamentManager::Authorize(BSTR password, int* success) {
	if (success == nullptr) {
		return E_POINTER;
	}

	std::wstring pass_str = BSTRToWstring(password);
	*success = pass_str == PASSWORD ? 1 : 0;
	return S_OK;
}

STDMETHODIMP TournamentManager::Subscribe(ITournamentEvents* p_client_sink, Role role, int table) {
	if (p_client_sink == nullptr) {
		return E_POINTER;
	}

	std::lock_guard<std::mutex> lock(mu);
	p_client_sink->AddRef();
	ClientConn conn{ p_client_sink, role, table };

	if (conn.role == ROLE_ADMIN) {
		conn.table = -1;
	}

	conn_list.push_back(conn);
	return S_OK;

}

STDMETHODIMP TournamentManager::Unsubscribe(ITournamentEvents* p_client_sink) {
	if (p_client_sink == nullptr) {
		return E_POINTER;
	}

	std::lock_guard<std::mutex> lock(mu);

	auto it = std::find_if(
		conn_list.begin(),
		conn_list.end(),
		[p_client_sink](const ClientConn& conn) { return conn.client_sink == p_client_sink; }
	);
	
	if (it != conn_list.end()) {
		it->client_sink->Release();
		conn_list.erase(it);
		return S_OK;
	}

	return E_INVALIDARG;
}

STDMETHODIMP TournamentManager::RegisterPlayer(BSTR playerName, int rating) {
	std::lock_guard<std::mutex> lock(mu);
	auto name = BSTRToWstring(playerName);

	for (const auto pl : players) {
		if (pl->name == name)
			return E_INVALIDARG;
	}

	auto p = new Player{ name, rating };
	players.push_back(p);
	SafePlayerStats stats{ p->name, -1, 0, 0 };
	player_stats[name] = stats;

	return S_OK;
}

void TournamentManager::UpdatePlayerStats(Player* p, int add_wins, int add_loses, int place) {
	auto& stats = player_stats[p->name];
	stats.wins += add_wins;
	stats.loses += add_loses;
	stats.place = (add_wins < add_loses || place == 1) ? place : -1;
}

void TournamentManager::NotifyMatchStarted(const Match& match) {
	BSTR bstr_p1 = WStringToBSTR(match.player1->name);
	BSTR bstr_p2 = WStringToBSTR(match.player2->name);

	for (auto it = conn_list.begin(); it != conn_list.end(); ) {
		if (it->role == ROLE_ADMIN || it->table == match.table) {
			HRESULT hr = it->client_sink->OnMatchStarted(bstr_p1, bstr_p2, match.table);
			if (FAILED(hr)) {
				if (hr == RPC_S_SERVER_UNAVAILABLE || hr == RPC_E_DISCONNECTED) {
					it->client_sink->Release();
					it = conn_list.erase(it);
					continue;
				}
			}
			it++;
		}
	}

	::SysFreeString(bstr_p1);
	::SysFreeString(bstr_p2);
}

void TournamentManager::NotifyGridUpdate() {
	for (auto it = conn_list.begin(); it != conn_list.end(); ) {
		if (it->role == ROLE_ADMIN) {
			HRESULT hr = it->client_sink->OnGridUpdated();
			if (FAILED(hr)) {
				if (hr == RPC_S_SERVER_UNAVAILABLE || hr == RPC_E_DISCONNECTED) {
					it->client_sink->Release();
					it = conn_list.erase(it);
					continue;
				}
			}

			it++;
		}
	}
}

void TournamentManager::NotifyTournamentFinished(std::wstring winner) {
	BSTR bstr_winner = WStringToBSTR(winner);

	for (auto it = conn_list.begin(); it != conn_list.end(); ) {
		HRESULT hr = it->client_sink->OnTournamentFinished(bstr_winner);
		if (FAILED(hr)) {
			if (hr == RPC_S_SERVER_UNAVAILABLE || hr == RPC_E_DISCONNECTED) {
				it->client_sink->Release();
				it = conn_list.erase(it);
				continue;
			}
		}

		it++;
	}

	::SysFreeString(bstr_winner);
}

STDMETHODIMP TournamentManager::GetPlayerStats(BSTR player_name, PlayerStats* stats) {
	if (stats == nullptr) {
		return E_POINTER;
	}

	std::wstring name = BSTRToWstring(player_name);
	if (!player_stats.count(name) == 0) {
		return E_INVALIDARG;
	}

	auto& curr_stats = player_stats[name];
	stats->name = player_name;
	stats->wins = curr_stats.wins;
	stats->loses = curr_stats.loses;
	stats->place = curr_stats.place;

	return S_OK;
}

STDMETHODIMP TournamentManager::StartTournament(int tables) {
	if (tables <= 0) {
		return E_INVALIDARG;
	}

	std::lock_guard<std::mutex> lock(mu);
	unsigned int p_num = players.size();

	if (p_num == 0) {
		return E_FAIL;
	}

	if (1 << (31 - __lzcnt(p_num)) != p_num) {
		return E_FAIL;
	}

	for (int i = 0; i < tables; i++) {
		free_tables.push(i);
	}

	// fill grid with matches
	grid.resize(p_num);
	for (int i = 0; i < static_cast<int>(p_num) / 2; i++) {
		auto& m = grid[p_num - i - 1];
		m.player1 = players[2 * i];
		m.player2 = players[2 * i + 1];

		if (!free_tables.empty()) {
			m.started = true;
			m.table = free_tables.front();
			free_tables.pop();
			NotifyMatchStarted(m);
		} else {
			awaited_matches.push(p_num - i - 1);
		}
	}

	return S_OK;
}

void TournamentManager::TryStartMatches() {
	while (!awaited_matches.empty() && !free_tables.empty()) {
		auto& new_match = grid[awaited_matches.front()];
		int table = free_tables.front();

		new_match.started = true;
		new_match.table = table;
		awaited_matches.pop();
		free_tables.pop();

		NotifyMatchStarted(new_match);
	}
}

STDMETHODIMP TournamentManager::SaveMatchResults(BSTR player1, BSTR player2, int score1, int score2) {
	if (score1 == score2) {
		return E_INVALIDARG;
	}

	auto p_name1 = BSTRToWstring(player1);
	auto p_name2 = BSTRToWstring(player2);
	int grid_size = grid.size();

	std::lock_guard<std::mutex> lock(mu);
	for (int i = 1; i < grid_size; i++) {
		auto& m = grid[i];
		if (!(m.player1 && m.player2)) {
			continue;
		}

		if ((m.player1->name == p_name1 && m.player2->name == p_name2) ||
			(m.player1->name == p_name2 && m.player2->name == p_name1)) {

			if (m.finished) {
				return E_FAIL;
			}

			m.finished = true;
			m.score1 = score1;
			m.score2 = score2;

			UpdatePlayerStats(m.player1, score1, score2, i / 2 + 1);
			UpdatePlayerStats(m.player2, score2, score1, i / 2 + 1);
			free_tables.push(m.table);

			int next_match_i = i >> 1;
			auto& next_m = grid[next_match_i];

			if (next_match_i > 0) {
				auto& next_m = grid[next_match_i];

				if (!next_m.player1) {
					next_m.player1 = m.GetWinner();
				} else {
					next_m.player2 = m.GetWinner();
					next_m.score1 = next_m.score2 = 0;
					awaited_matches.push(next_match_i);
					TryStartMatches();
				}
			} else {
				NotifyTournamentFinished(m.GetWinner()->name);
				return S_OK;
			}

			NotifyGridUpdate();
			return S_OK;
		}
	}

	return E_INVALIDARG;
}

