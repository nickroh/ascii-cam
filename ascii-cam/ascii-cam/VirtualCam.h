#pragma once
#include <windows.h>
#include <mfvirtualcamera.h>
#include <mfapi.h>
#include <string>


class VirtualCam {
public:
    VirtualCam(const std::wstring& name);
    ~VirtualCam();

    bool Initialize();
    void Shutdown();

private:
    std::wstring m_name;
    IMFVirtualCamera* m_camera;
};