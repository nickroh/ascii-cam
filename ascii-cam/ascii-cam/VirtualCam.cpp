#include "VirtualCam.h"
#include <iostream>
#include <combaseapi.h> // CoInitializeEx
#include <initguid.h> 
#include "CLSID.h"

#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "Mf.lib")
#pragma comment(lib, "mfsensorgroup.lib")


VirtualCam::VirtualCam(const std::wstring& name)
    : m_name(name), m_camera(nullptr) {
}

VirtualCam::~VirtualCam() {
    Shutdown();
}

bool VirtualCam::Initialize()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;

    // 🔥 CLSID → 문자열 변환
    wchar_t clsidStr[64];
    StringFromGUID2(CLSID_MyMediaSource, clsidStr, 64);

    hr = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser,
        m_name.c_str(),
        clsidStr,   // 🔥 핵심
        nullptr,
        0,
        &m_camera
    );

    if (FAILED(hr)) {
        std::wcerr << L"CreateVirtualCamera failed: " << hr << std::endl;
        return false;
    }

    // 🔥 중요: source 안 넘김
    hr = m_camera->Start(nullptr);

    if (FAILED(hr)) {
        std::wcerr << L"Start failed: " << hr << std::endl;
        return false;
    }

    std::wcout << L"Virtual Camera started\n";
    return true;
}

void VirtualCam::Shutdown()
{
    if (m_camera) {
        m_camera->Stop();
        m_camera->Remove(); // 🔥 장치 제거
        m_camera->Release();
        m_camera = nullptr;
    }

    MFShutdown();
    CoUninitialize();
}