#include "TournamentEvents.h"
#include "Utils.h"
#include <string>
#include <iostream>


TournamentEvents::TournamentEvents(HANDLE exit_event) : exit_event(exit_event), ref_count(1) {}

STDMETHODIMP TournamentEvents::QueryInterface(REFIID riid, void** ppvObject) {
	if (ppvObject == nullptr) {
		return E_POINTER;
	}

	*ppvObject = nullptr;
	if (riid == IID_IUnknown) {
		*ppvObject = static_cast<IUnknown*>(this);
	} else if (riid == IID_ITournamentEvents) {
		*ppvObject = static_cast<ITournamentEvents*>(this);
	} else {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) TournamentEvents::AddRef() {
	return ++ref_count;
}

STDMETHODIMP_(ULONG) TournamentEvents::Release() {
	ULONG rem_refs = --ref_count;
	if (rem_refs == 0) {
		delete this;
	}

	return rem_refs;
}

STDMETHODIMP TournamentEvents::OnMatchStarted(BSTR player1, BSTR player2, int table) {
	std::wstring p1_name = BSTRToWstring(player1);
	std::wstring p2_name = BSTRToWstring(player2);

	std::wcout 
		<< L"Match started between " 
		<< p1_name << L" and " << p2_name 
		<< L" (" << table << L")" << std::endl;

	return S_OK;
}

STDMETHODIMP TournamentEvents::OnGridUpdated() {
	std::wcout << "Tournament grid got updated" << std::endl;
	return S_OK;
}

STDMETHODIMP TournamentEvents::OnTournamentFinished(BSTR winnerName) {
	std::wstring winner = BSTRToWstring(winnerName);
	std::wcout << L"Tournament finished! Winner is " << winner << std::endl;
	
	if (exit_event != nullptr) {
		::SetEvent(exit_event);
	}

	return S_OK;
}