// MediaSource.h
#pragma once
#include <mfidl.h>

class MyMediaSource : public IMFMediaSource {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    STDMETHODIMP GetCharacteristics(DWORD*) override { return S_OK; }
    STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor**) override { return E_NOTIMPL; }
    STDMETHODIMP Start(IMFPresentationDescriptor*, const GUID*, const PROPVARIANT*) override { return S_OK; }
    STDMETHODIMP Stop() override { return S_OK; }
    STDMETHODIMP Pause() override { return E_NOTIMPL; }
    STDMETHODIMP Shutdown() override { return S_OK; }

    STDMETHODIMP BeginGetEvent(IMFAsyncCallback*, IUnknown*) override { return E_NOTIMPL; }
    STDMETHODIMP EndGetEvent(IMFAsyncResult*, IMFMediaEvent**) override { return E_NOTIMPL; }
    STDMETHODIMP GetEvent(DWORD, IMFMediaEvent**) override { return E_NOTIMPL; }
    STDMETHODIMP QueueEvent(MediaEventType, REFGUID, HRESULT, const PROPVARIANT*) override { return E_NOTIMPL; }

private:
    long m_ref = 1;
};