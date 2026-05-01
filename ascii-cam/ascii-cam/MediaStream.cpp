#include "MediaStream.h"
#include "MediaSource.h"
#include <mfapi.h>

MyMediaStream::MyMediaStream(MyMediaSource* src)
    : m_ref(1), m_source(src), m_queue(nullptr)
{
    MFCreateEventQueue(&m_queue);
}

MyMediaStream::~MyMediaStream() {
    if (m_queue) m_queue->Release();
}

void MyMediaStream::QueueSample(IMFSample* sample)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    sample->AddRef();
    m_samples.push(sample);
}

HRESULT MyMediaStream::RequestSample(IUnknown*)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_samples.empty()) return S_OK;

    IMFSample* s = m_samples.front();
    m_samples.pop();

    return m_queue->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, s);
}

HRESULT MyMediaStream::GetMediaSource(IMFMediaSource** pp)
{
    *pp = (IMFMediaSource*)m_source;
    m_source->AddRef();
    return S_OK;
}

// Event passthrough
HRESULT MyMediaStream::BeginGetEvent(IMFAsyncCallback* c, IUnknown* s) { return m_queue->BeginGetEvent(c, s); }
HRESULT MyMediaStream::EndGetEvent(IMFAsyncResult* r, IMFMediaEvent** e) { return m_queue->EndGetEvent(r, e); }
HRESULT MyMediaStream::GetEvent(DWORD f, IMFMediaEvent** e) { return m_queue->GetEvent(f, e); }
HRESULT MyMediaStream::QueueEvent(MediaEventType t, REFGUID g, HRESULT h, const PROPVARIANT* v) {
    return m_queue->QueueEventParamVar(t, g, h, v);
}

// IUnknown
ULONG MyMediaStream::AddRef() { return InterlockedIncrement(&m_ref); }
ULONG MyMediaStream::Release() {
    ULONG c = InterlockedDecrement(&m_ref);
    if (!c) delete this;
    return c;
}
HRESULT MyMediaStream::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IMFMediaStream || riid == IID_IMFMediaEventGenerator) {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}