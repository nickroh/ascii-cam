#include "pch.h"
#include "WebcamCapture.h"

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

    // 4. Source Reader 생성
    hr = MFCreateSourceReaderFromMediaSource(_source.Get(), nullptr, &_reader);
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

    if (!targetIsYUY2)
    {
        wil::com_ptr_nothrow<IMFMediaType> outputType;
        hr = MFCreateMediaType(&outputType);
        if (FAILED(hr)) return hr;

        // 1. 기본 타입 설정
        hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (FAILED(hr)) return hr;

        hr = outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);
        if (FAILED(hr)) return hr;

        // 2. [필수 추가] 디코더가 출력할 해상도 지정 (1280x720)
        hr = MFSetAttributeSize(outputType.get(), MF_MT_FRAME_SIZE, width, height);
        if (FAILED(hr)) return hr;

        // 3. [필수 추가] 디코더가 출력할 프레임레이트 지정 (30fps)
        hr = MFSetAttributeRatio(outputType.get(), MF_MT_FRAME_RATE, maxFpsFound, 1);
        if (FAILED(hr)) return hr;

        // 4. 완벽하게 구성된 YUY2 출력 MediaType 적용
        hr = _reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType.get());
        if (FAILED(hr)) return hr;

        WINTRACE(L"[Wcam] Auto Decoder Configured: Native (%u fps) -> YUY2 Output (%ux%u)", maxFpsFound, width, height);
    }
    else
    {
        WINTRACE(L"[Wcam] Native YUY2 Selected (%u fps)", maxFpsFound);
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