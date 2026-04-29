#include "MediaStream.h"
#include <mfapi.h>
#include <mferror.h>
#include <iostream>

MyMediaStream::MyMediaStream(MyMediaSource* source)
    : m_refCount(1), m_source(source), m_streamDesc(nullptr), m_eventQueue(nullptr)
{
    MFCreateEventQueue(&m_eventQueue);
}

MyMediaStream::~MyMediaStream()
{
    if (m_eventQueue) m_eventQueue->Release();
    if (m_streamDesc) m_streamDesc->Release();
}

//////////////////////////////////////////////////////////////
// IUnknown
//////////////////////////////////////////////////////////////

ULONG MyMediaStream::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

ULONG MyMediaStream::Release()
{
    ULONG count = InterlockedDecrement(&m_refCount);
    if (count == 0) delete this;
    return count;
}

HRESULT MyMediaStream::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IMFMediaStream || riid == IID_IMFMediaEventGenerator)
    {
        *ppv = static_cast<IMFMediaStream*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////
// IMFMediaEventGenerator
//////////////////////////////////////////////////////////////

HRESULT MyMediaStream::BeginGetEvent(IMFAsyncCallback* pCallback, IUnknown* punkState)
{
    return m_eventQueue->BeginGetEvent(pCallback, punkState);
}

HRESULT MyMediaStream::EndGetEvent(IMFAsyncResult* pResult, IMFMediaEvent** ppEvent)
{
    return m_eventQueue->EndGetEvent(pResult, ppEvent);
}

HRESULT MyMediaStream::GetEvent(DWORD dwFlags, IMFMediaEvent** ppEvent)
{
    return m_eventQueue->GetEvent(dwFlags, ppEvent);
}

HRESULT MyMediaStream::QueueEvent(MediaEventType met, REFGUID guidExtendedType,
    HRESULT hrStatus, const PROPVARIANT* pvValue)
{
    return m_eventQueue->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);
}

//////////////////////////////////////////////////////////////
// IMFMediaStream
//////////////////////////////////////////////////////////////

HRESULT MyMediaStream::GetMediaSource(IMFMediaSource** ppMediaSource)
{
    *ppMediaSource = (IMFMediaSource*)m_source;
    if (m_source) m_source->AddRef();
    return S_OK;
}

HRESULT MyMediaStream::GetStreamDescriptor(IMFStreamDescriptor** ppStreamDescriptor)
{
    if (!m_streamDesc) return E_FAIL;

    *ppStreamDescriptor = m_streamDesc;
    m_streamDesc->AddRef();
    return S_OK;
}

HRESULT MyMediaStream::RequestSample(IUnknown* pToken)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_sampleQueue.empty())
    {
        // 아직 샘플 없음 → 기다림
        return S_OK;
    }

    IMFSample* sample = m_sampleQueue.front();
    m_sampleQueue.pop();

    // 🔥 핵심: 샘플 전달 이벤트
    return m_eventQueue->QueueEventParamUnk(
        MEMediaSample,
        GUID_NULL,
        S_OK,
        sample
    );
}

//////////////////////////////////////////////////////////////
// 🔥 우리가 만든 핵심 함수
//////////////////////////////////////////////////////////////

void MyMediaStream::QueueSample(IMFSample* sample)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (sample)
    {
        sample->AddRef();
        m_sampleQueue.push(sample);
    }
}