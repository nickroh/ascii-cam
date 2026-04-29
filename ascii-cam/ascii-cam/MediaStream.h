#pragma once

#include <mfidl.h>
#include <mfobjects.h>
#include <queue>
#include <mutex>

class MyMediaSource; // forward

class MyMediaStream : public IMFMediaStream
{
public:
    MyMediaStream(MyMediaSource* source);
    virtual ~MyMediaStream();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFMediaEventGenerator
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState);
    STDMETHODIMP EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent);
    STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent);
    STDMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType,
        HRESULT hrStatus, const PROPVARIANT* pvValue);

    // IMFMediaStream
    STDMETHODIMP GetMediaSource(IMFMediaSource** ppMediaSource);
    STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor** ppStreamDescriptor);
    STDMETHODIMP RequestSample(IUnknown* pToken);

    // 🔥 우리가 추가하는 함수
    void QueueSample(IMFSample* sample);

private:
    long m_refCount;

    MyMediaSource* m_source;
    IMFStreamDescriptor* m_streamDesc;
    IMFMediaEventQueue* m_eventQueue;

    std::queue<IMFSample*> m_sampleQueue;
    std::mutex m_mutex;
};