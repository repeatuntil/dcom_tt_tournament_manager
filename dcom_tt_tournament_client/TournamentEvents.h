#pragma once

#include "tournament_server_h.h"
#include <atomic>


class TournamentEvents : public ITournamentEvents {
public:
	TournamentEvents(HANDLE exit_event);
	~TournamentEvents() = default;

	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
	STDMETHODIMP_(ULONG) AddRef() override;
	STDMETHODIMP_(ULONG) Release() override;

	STDMETHODIMP OnMatchStarted(BSTR player1, BSTR player2, int table) override;
	STDMETHODIMP OnGridUpdated() override;
	STDMETHODIMP OnTournamentFinished(BSTR winnerName) override;
private:
	std::atomic<unsigned long> ref_count;
	HANDLE exit_event;
};