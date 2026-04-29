#pragma once
#include <windows.h>
#include <mfvirtualcamera.h>
#include <mfapi.h>
#include <string>
#include <guiddef.h>

inline constexpr GUID CLSID_MyVirtualCamera =
{ 0x88c15e7c, 0x020f, 0x463f, { 0xa6, 0x2f, 0x06, 0x39, 0x91, 0x20, 0x61, 0xa0 } };

class VirtualCam {
public:
    VirtualCam(const std::wstring& friendlyName);
    ~VirtualCam();

    // 가상 카메라 장치를 시스템에 등록하고 활성화
    bool Initialize();

    // 서비스 중지 및 장치 제거
    void Shutdown();

    bool IsActive() const { return m_pVirtualCamera != nullptr; }

private:
    std::wstring m_friendlyName;
    IMFVirtualCamera* m_pVirtualCamera;

    // 가상 카메라를 식별하기 위한 고유 GUID (적절한 GUID로 교체 가능)
    // {88C15E7C-020F-463F-A62F-0639912061A0}
    //const GUID m_cameraCategory = KSCATEGORY_VIDEO_CAMERA;
};