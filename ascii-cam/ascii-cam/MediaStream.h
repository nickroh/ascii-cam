#pragma once
#include <mfidl.h>
#include <queue>
#include <mutex>

class MyMediaSource;

class MyMediaStream : public IMFMediaStream {
public:
    MyMediaStream(MyMediaSource* source);
    ~MyMediaStream();

    void QueueSample(IMFSample* sample);

    // IMFMediaStream
    STDMETHODIMP GetMediaSource(IMFMediaSource** pp) override;
    STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor**) override { return E_NOTIMPL; }
    STDMETHODIMP RequestSample(IUnknown* token) override;

    // IMFMediaEventGenerator
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback*, IUnknown*) override;
    STDMETHODIMP EndGetEvent(IMFAsyncResult*, IMFMediaEvent**) override;
    STDMETHODIMP GetEvent(DWORD, IMFMediaEvent**) override;
    STDMETHODIMP QueueEvent(MediaEventType, REFGUID, HRESULT, const PROPVARIANT*) override;

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void**) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

private:
    long m_ref;
    MyMediaSource* m_source;
    IMFMediaEventQueue* m_queue;
    std::queue<IMFSample*> m_samples;
    std::mutex m_mutex;
};