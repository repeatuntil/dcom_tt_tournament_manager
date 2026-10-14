#include "TournamentManager.h"
#include <iostream>

STDMETHODIMP TournamentClassFactory::QueryInterface(REFIID riid, void** ppvObject) {
	if (ppvObject == nullptr) {
		return E_POINTER;
	}

	*ppvObject = nullptr;
	if (riid == IID_IUnknown) {
		*ppvObject = static_cast<IUnknown*>(this);
	} else if (riid == IID_IClassFactory) {
		*ppvObject = static_cast<IClassFactory*>(this);
	} else {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) TournamentClassFactory::AddRef() {
	return 1;
}

STDMETHODIMP_(ULONG) TournamentClassFactory::Release() {
	return 1;
}

STDMETHODIMP TournamentClassFactory::CreateInstance(IUnknown* pUnk, REFIID riid, void** ppvObject) {
	if (pUnk != nullptr) {
		*ppvObject = nullptr;
		return CLASS_E_NOAGGREGATION;
	}

	TournamentManager* tournament_manager = new TournamentManager();
	HRESULT hr = tournament_manager->QueryInterface(riid, ppvObject);
	if (FAILED(hr)) {
		std::wcout << L"Can't create tManager instance" << std::endl;
		delete tournament_manager;
	} 
	/*else {
		std::wcout << L"release (from factory)" << std::endl;
		tournament_manager->Release();
	}*/

	return hr;
}

STDMETHODIMP TournamentClassFactory::LockServer(BOOL fLock) {
	if (fLock) {
		locks_count++;
	} else if (--locks_count == 0 && TournamentManager::obj_count == 0) {
		::PostQuitMessage(0);
	}

	return S_OK;
}