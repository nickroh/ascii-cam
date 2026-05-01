// MediaSource.cpp
#include "MediaSource.h"

HRESULT MyMediaSource::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IMFMediaSource) {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

ULONG MyMediaSource::AddRef() { return InterlockedIncrement(&m_ref); }
ULONG MyMediaSource::Release() {
    ULONG c = InterlockedDecrement(&m_ref);
    if (!c) delete this;
    return c;
}