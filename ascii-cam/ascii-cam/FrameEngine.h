#pragma once

#include <mfapi.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <windows.h>

class FrameEngine
{
public:
    FrameEngine(int width = 640, int height = 480, int fps = 30);
    ~FrameEngine();

    // 초기화
    bool Initialize();

    // 프레임 생성
    IMFSample* CreateFrame();

    // FPS 관련
    LONGLONG GetFrameDuration() const;

private:
    int m_width;
    int m_height;
    int m_fps;

    LONGLONG m_frameDuration; // 100ns 단위
    LONGLONG m_timestamp;
};