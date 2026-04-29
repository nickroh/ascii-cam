#include "FrameEngine.h"
#include <iostream>

FrameEngine::FrameEngine(int width, int height, int fps)
    : m_width(width),
    m_height(height),
    m_fps(fps),
    m_timestamp(0)
{
    // 1초 = 10,000,000 (100ns 단위)
    m_frameDuration = 10'000'000 / fps;
}

FrameEngine::~FrameEngine()
{
}

bool FrameEngine::Initialize()
{
    // 지금은 특별히 할 건 없음 (확장 대비)
    return true;
}

LONGLONG FrameEngine::GetFrameDuration() const
{
    return m_frameDuration;
}

IMFSample* FrameEngine::CreateFrame()
{
    IMFSample* sample = nullptr;
    IMFMediaBuffer* buffer = nullptr;

    int ySize = m_width * m_height;
    int uvSize = ySize / 2;
    int bufferSize = ySize + uvSize;

    if (FAILED(MFCreateSample(&sample)))
        return nullptr;

    if (FAILED(MFCreateMemoryBuffer(bufferSize, &buffer)))
    {
        sample->Release();
        return nullptr;
    }

    BYTE* data = nullptr;
    if (FAILED(buffer->Lock(&data, nullptr, nullptr)))
    {
        buffer->Release();
        sample->Release();
        return nullptr;
    }

    // Y plane
    memset(data, 128, ySize);

    // UV plane
    memset(data + ySize, 128, uvSize);

    buffer->Unlock();
    buffer->SetCurrentLength(bufferSize);

    if (FAILED(sample->AddBuffer(buffer)))
    {
        buffer->Release();
        sample->Release();
        return nullptr;
    }

    sample->SetSampleTime(m_timestamp);
    sample->SetSampleDuration(m_frameDuration);

    m_timestamp += m_frameDuration;

    buffer->Release();
    return sample;
}