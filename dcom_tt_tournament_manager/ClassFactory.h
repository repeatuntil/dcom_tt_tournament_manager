#pragma once
#include "TournamentManager.h"
#include <unknwn.h>
#include <atomic>

class TournamentClassFactory : public IClassFactory {
    friend class TournamentManager;
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override;
    STDMETHODIMP LockServer(BOOL fLock) override;

private:
    inline static std::atomic<unsigned long> locks_count{0};
};
