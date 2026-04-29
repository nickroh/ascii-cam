#include "VirtualCam.h"
#include <iostream>
#include <combaseapi.h> // CoInitializeEx

#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "Mf.lib")
#pragma comment(lib, "mfsensorgroup.lib")

VirtualCam::VirtualCam(const std::wstring& friendlyName)
    : m_friendlyName(friendlyName),
    m_pVirtualCamera(nullptr)
{
}

VirtualCam::~VirtualCam() {
    Shutdown();
}

bool VirtualCam::Initialize() {
    HRESULT hr;

    // Init COM Library
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); // Can be accessed from mulitple threads
    if (FAILED(hr)) {
        std::wcerr << L"COM init failed\n";
        return false;
    }

    // Start Media Foundation
    hr = MFStartup(MF_VERSION); // Auto select proper version for currently installed SDK
    if (FAILED(hr)) {
        std::wcerr << L"MFStartup failed\n";
        CoUninitialize();
        return false;
    }

    // Convert GUID to string
    wchar_t guidStr[64];
    if (StringFromGUID2(CLSID_MyVirtualCamera, guidStr, 64) == 0) {
        std::wcerr << L"GUID conversion failed\n";
        Shutdown();
        return false;
    }

    // Create Virtual Cam Instance
    hr = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource, // Specify CAM is virtual - software based
        MFVirtualCameraLifetime_Session, // CAM only available while program is alive
        MFVirtualCameraAccess_AllUsers, // Currently logged in user can use cam
        m_friendlyName.c_str(), // Displayed CAM name
        guidStr,               
        nullptr,
        0,
        &m_pVirtualCamera
    );

    if (FAILED(hr) || !m_pVirtualCamera) {
        std::wcerr << L"Failed to create virtual camera: " << hr << std::endl;
        Shutdown();
        return false;
    }

    // Start CAM
    hr = m_pVirtualCamera->Start(nullptr);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to start virtual camera: " << hr << std::endl;
        Shutdown();
        return false;
    }

    std::wcout << L"Virtual Camera '" << m_friendlyName << L"' is now active.\n";
    return true;
}

void VirtualCam::Shutdown() {
    if (m_pVirtualCamera) {
        m_pVirtualCamera->Stop();
        m_pVirtualCamera->Release();
        m_pVirtualCamera = nullptr;
    }

    MFShutdown();
    CoUninitialize();
}