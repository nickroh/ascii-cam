#include "pch.h"
#include "WebcamCapture.h"
#include <d3d11.h> 
#include <mfapi.h>

#include <mferror.h>

using Microsoft::WRL::ComPtr;

WebcamCapture::WebcamCapture()
{
}

WebcamCapture::~WebcamCapture()
{
    Shutdown();
}
HRESULT WebcamCapture::Initialize(
    UINT width,
    UINT height,
    UINT fps
)
{
    HRESULT hr = S_OK;

    _width = width;
    _height = height;
    _fps = fps;

    // 1. 디바이스 속성 생성
    wil::com_ptr_nothrow<IMFAttributes> attributes;
    hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) return hr;

    hr = attributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
    if (FAILED(hr)) return hr;

    // 2. 비디오 캡처 장치 열거
    IMFActivate** devices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(attributes.get(), &devices, &count);
    if (FAILED(hr)) return hr;

    if (count == 0)
    {
        CoTaskMemFree(devices);
        return E_FAIL;
    }

    // 3. 실제 카메라 찾기 (가상 카메라 제외)
    bool found = false;
    for (UINT32 i = 0; i < count; i++)
    {
        WCHAR* name = nullptr;
        UINT32 cchName = 0;

        hr = devices[i]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &name,
            &cchName
        );
        if (FAILED(hr)) continue;

        WINTRACE(L"Camera Found: %s", name);

        bool isVirtual = (wcsstr(name, L"Virtual") != nullptr) ||
                         (wcsstr(name, L"가상") != nullptr);

        if (isVirtual)
        {
            WINTRACE(L"Skipping Virtual Camera: %s", name);
            CoTaskMemFree(name);
            continue;
        }

        hr = devices[i]->ActivateObject(IID_PPV_ARGS(&_source));
        CoTaskMemFree(name);

        if (SUCCEEDED(hr))
        {
            found = true;
            break;
        }
    }

    // 디바이스 리소스 해제
    for (UINT32 i = 0; i < count; i++)
    {
        devices[i]->Release();
    }
    CoTaskMemFree(devices);

    if (!found) return E_FAIL;

    // 3-2. D3D 11 & DXGI MGM 생성 시도
    wil::com_ptr_nothrow<IMFDXGIDeviceManager> dxgiManager;
    UINT resetToken = 0;
    bool isGpuAccelerated = false;

    wil::com_ptr_nothrow<ID3D11Device> d3d11Device;
    wil::com_ptr_nothrow<ID3D11DeviceContext> d3d11Context;
    D3D_FEATURE_LEVEL featureLevel;

    // D3D11 디바이스 생성 시도
    HRESULT hrD3D = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
        nullptr, 0,
        D3D11_SDK_VERSION,
        &d3d11Device,
        &featureLevel,
        &d3d11Context
    );

    if (SUCCEEDED(hrD3D) && d3d11Device)
    {
        // Media Foundation 스레드 안전성 확보
        wil::com_ptr_nothrow<ID3D10Multithread> multiThread;
        if (SUCCEEDED(d3d11Device->QueryInterface(IID_PPV_ARGS(&multiThread))))
        {
            multiThread->SetMultithreadProtected(TRUE);
        }

        // DXGI Device Manager 생성 시도
        HRESULT hrDXGI = MFCreateDXGIDeviceManager(&resetToken, &dxgiManager);
        if (SUCCEEDED(hrDXGI) && dxgiManager)
        {
            if (SUCCEEDED(dxgiManager->ResetDevice(d3d11Device.get(), resetToken)))
            {
                isGpuAccelerated = true;
                WINTRACE(L"[Wcam] GPU Acceleration Enabled (Direct3D 11).");
            }
            else
            {
                dxgiManager.reset();
                WINTRACE(L"[Wcam] DXGI ResetDevice failed. Falling back to CPU mode.");
            }
        }
        else
        {
            WINTRACE(L"[Wcam] DXGI Device Manager creation failed. Falling back to CPU mode.");
        }
    }
    else
    {
        WINTRACE(L"[Wcam] D3D11 Device creation failed (hr = 0x%08X). Falling back to CPU mode.", hrD3D);
    }

    // 3-3. apply CPU/CPU attribute
    wil::com_ptr_nothrow<IMFAttributes> readerAttributes;
    hr = MFCreateAttributes(&readerAttributes, 4);
    if (FAILED(hr)) return hr;

    // 하드웨어 디코더 가속 요청
    readerAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    readerAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);

    // GPU가 사용 가능한 경우 DXGI Manager 바인딩
    if (isGpuAccelerated && dxgiManager)
    {
        readerAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, dxgiManager.get());
    }


    // 4. Source Reader 생성
    hr = MFCreateSourceReaderFromMediaSource(_source.Get(), readerAttributes.get(), &_reader);

    // GPU 속성 결합으로 생성이 실패한 경우 CPU 전용 속성으로 재시도 (2차 Fallback)
    if (FAILED(hr) && isGpuAccelerated)
    {
        WINTRACE(L"[Wcam] Reader creation failed with GPU attributes. Retrying in CPU mode...");
        readerAttributes->DeleteItem(MF_SOURCE_READER_D3D_MANAGER);
        isGpuAccelerated = false;
        dxgiManager.reset();

        hr = MFCreateSourceReaderFromMediaSource(_source.Get(), readerAttributes.get(), &_reader);
    }

    if (FAILED(hr)) return hr;

    // 5. [개선] 단일 루프로 최고 FPS 포맷 탐색
    DWORD typeIndex = 0;
    DWORD bestTypeIndex = DWORD_MAX;
    UINT32 maxFpsFound = 0;
    bool bestIsYUY2 = false;

    // Fallback용 변수 (해상도가 안 맞을 때 대비)
    DWORD fallbackTypeIndex = DWORD_MAX;
    UINT32 fallbackMaxFps = 0;

    while (true)
    {
        wil::com_ptr_nothrow<IMFMediaType> nativeType;
        hr = _reader->GetNativeMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            typeIndex,
            &nativeType
        );

        if (FAILED(hr)) break; // 탐색 완료

        GUID subtype = GUID_NULL;
        UINT32 w = 0, h = 0;
        UINT32 num = 0, den = 0;

        nativeType->GetGUID(MF_MT_SUBTYPE, &subtype);
        MFGetAttributeSize(nativeType.get(), MF_MT_FRAME_SIZE, &w, &h);
        MFGetAttributeRatio(nativeType.get(), MF_MT_FRAME_RATE, &num, &den);

        UINT32 currentFps = (den > 0) ? (num / den) : 0;

        WCHAR guidText[64];
        StringFromGUID2(subtype, guidText, ARRAYSIZE(guidText));
        WINTRACE(L"[Wcam] NativeType %u -> %s %ux%u @ %ufps", typeIndex, guidText, w, h, currentFps);

        // 조건 A: 요청한 해상도(width x height)와 일치하는 경우
        if (w == width && h == height)
        {
            // 1. 더 높은 FPS를 찾았거나
            // 2. FPS가 같은데 디코딩이 필요 없는 YUY2인 경우 우선 선택
            if (currentFps > maxFpsFound ||
               (currentFps == maxFpsFound && subtype == MFVideoFormat_YUY2 && !bestIsYUY2))
            {
                maxFpsFound = currentFps;
                bestTypeIndex = typeIndex;
                bestIsYUY2 = (subtype == MFVideoFormat_YUY2);
            }
        }

        // 조건 B: Fallback (YUY2 중 가장 FPS가 높은 것)
        if (subtype == MFVideoFormat_YUY2 && currentFps > fallbackMaxFps)
        {
            fallbackMaxFps = currentFps;
            fallbackTypeIndex = typeIndex;
        }

        typeIndex++;
    }

    // 6. [개선] 최적의 포맷 적용 및 자동 YUY2 디코더 설정
    DWORD targetIndex = bestTypeIndex;
    bool targetIsYUY2 = bestIsYUY2;

    // 요청 해상도가 없을 경우 Fallback 적용
    if (targetIndex == DWORD_MAX)
    {
        if (fallbackTypeIndex != DWORD_MAX)
        {
            targetIndex = fallbackTypeIndex;
            targetIsYUY2 = true;
            WINTRACE(L"[Wcam] Exact resolution not found. Using Fallback YUY2.");
        }
        else
        {
            WINTRACE(L"[Wcam] No compatible format found.");
            return E_FAIL;
        }
    }

    // A. 카메라 하드웨어 출력 포맷 설정 (예: 30fps MJPEG 선택)
    wil::com_ptr_nothrow<IMFMediaType> selectedNativeType;
    hr = _reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, targetIndex, &selectedNativeType);
    if (FAILED(hr)) return hr;

    hr = _reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, selectedNativeType.get());
    if (FAILED(hr)) return hr;

    // Native 포맷이 YUY2/NV12가 아닌 경우 (예: MJPEG) -> 항상 NV12로 디코딩
    if (!targetIsYUY2)
    {
        wil::com_ptr_nothrow<IMFMediaType> outputType;
        hr = MFCreateMediaType(&outputType);
        if (FAILED(hr)) return hr;

        hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (FAILED(hr)) return hr;

        // [통일] GPU/CPU 구분 없이 항상 NV12로 출력 세팅
        hr = outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
        if (FAILED(hr)) return hr;

        hr = MFSetAttributeSize(outputType.get(), MF_MT_FRAME_SIZE, width, height);
        if (FAILED(hr)) return hr;

        hr = MFSetAttributeRatio(outputType.get(), MF_MT_FRAME_RATE, maxFpsFound, 1);
        if (FAILED(hr)) return hr;

        // 디코더 출력 포맷 적용
        hr = _reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType.get());
        if (FAILED(hr)) return hr;

        WINTRACE(L"[Wcam] Decoder Configured (%s): Native (%u fps) -> Output (NV12, %ux%u)",
            isGpuAccelerated ? L"GPU HW" : L"CPU SW", maxFpsFound, width, height);
    }

    return S_OK;
}

HRESULT WebcamCapture::GetFrame(IMFSample** sample)
{
    RETURN_HR_IF_NULL(E_POINTER, sample);

    *sample = nullptr;

    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG timestamp = 0;

    HRESULT hr = _reader->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &streamIndex,
        &flags,
        &timestamp,
        sample
    );

    RETURN_IF_FAILED(hr);

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
        return E_FAIL;
    }

    if (!*sample)
    {
        return E_FAIL;
    }

    return S_OK;
}

void WebcamCapture::Shutdown()
{
    _reader.Reset();

    if (_source)
    {
        _source->Shutdown();
        _source.Reset();
    }
}