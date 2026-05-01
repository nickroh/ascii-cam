#include "ClassFactory.h"
#include "MediaSource.h"

STDMETHODIMP ClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IClassFactory)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

ULONG ClassFactory::AddRef() { return 2; }
ULONG ClassFactory::Release() { return 1; }

STDMETHODIMP ClassFactory::CreateInstance(IUnknown*, REFIID riid, void** ppv)
{
    auto src = new MyMediaSource();
    return src->QueryInterface(riid, ppv);
}

STDMETHODIMP ClassFactory::LockServer(BOOL)
{
    return S_OK;
}