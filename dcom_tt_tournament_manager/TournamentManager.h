#pragma once
#include "tournament_server_h.h"
#include "ClassFactory.h"
#include <atomic>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <string>


struct ClientConn {
	ITournamentEvents* client_sink;
	Role role;
	int table;
};

struct Player {
	std::wstring name;
	int rating;
};

struct SafePlayerStats {
	std::wstring name;
	int place;
	int wins;
	int loses;
};

struct Match {
	Player* player1;
	Player* player2;
	int table;
	int score1;
	int score2;
	bool started;
	bool finished;
public:
	Player* GetWinner(void);
};


class TournamentManager : public ITournamentManager {
	friend class TournamentClassFactory;
public:
	TournamentManager();
	~TournamentManager();

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
	STDMETHODIMP_(ULONG) AddRef() override;
	STDMETHODIMP_(ULONG) Release() override;

	STDMETHODIMP RegisterPlayer(BSTR playerName, int rating) override;
	STDMETHODIMP StartTournament(int tables) override;
	STDMETHODIMP SaveMatchResults(BSTR player1, BSTR player2, int score1, int score2) override;
	STDMETHODIMP GetPlayerStats(BSTR player_name, PlayerStats* stats) override;
	STDMETHODIMP Authorize(BSTR password, int* success) override;
	STDMETHODIMP Subscribe(ITournamentEvents* p_client_sink, Role role, int table) override;
	STDMETHODIMP Unsubscribe(ITournamentEvents* p_client_sink) override;
private:
	std::atomic<ULONG> ref_count;
	inline static std::atomic<ULONG> obj_count{0};
	std::mutex mu;

	std::vector<ClientConn> conn_list;
	std::vector<Match> grid;
	std::vector<Player*> players;
	std::unordered_map<std::wstring, SafePlayerStats> player_stats;
	std::queue<int> free_tables;
	std::queue<int> awaited_matches;

	void TryStartMatches();
	void UpdatePlayerStats(Player* p, int add_wins, int add_loses, int place);
	void NotifyMatchStarted(const Match& match);
	void NotifyGridUpdate();
	void NotifyTournamentFinished(std::wstring winner);
};
