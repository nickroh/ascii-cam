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

    wil::com_ptr_nothrow<IMFAttributes> attributes;

    hr = MFCreateAttributes(
        &attributes,
        1
    );

    if (FAILED(hr))
        return hr;

    hr = attributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );

    if (FAILED(hr))
        return hr;

    IMFActivate** devices = nullptr;
    UINT32 count = 0;

    hr = MFEnumDeviceSources(
        attributes.get(),
        &devices,
        &count
    );

    if (FAILED(hr))
        return hr;

    if (count == 0)
    {
        CoTaskMemFree(devices);
        return E_FAIL;
    }

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

        if (FAILED(hr))
            continue;

        WINTRACE(L"Camera Found: %s", name);

        bool isVirtual =
            wcsstr(name, L"Virtual") != nullptr ||
            wcsstr(name, L"가상") != nullptr;

        if (isVirtual)
        {
            WINTRACE(
                L"Skipping Virtual Camera: %s",
                name
            );

            CoTaskMemFree(name);
            continue;
        }

        hr = devices[i]->ActivateObject(
            IID_PPV_ARGS(&_source)
        );

        CoTaskMemFree(name);

        if (SUCCEEDED(hr))
        {
            found = true;
            break;
        }
    }

    for (UINT32 i = 0; i < count; i++)
    {
        devices[i]->Release();
    }

    CoTaskMemFree(devices);

    if (!found)
    {
        return E_FAIL;
    }

    hr = MFCreateSourceReaderFromMediaSource(
        _source.Get(),
        nullptr,
        &_reader
    );

    if (FAILED(hr))
        return hr;

    //
    // enumerate native types
    //
    DWORD typeIndex = 0;

    while (true)
    {
        wil::com_ptr_nothrow<IMFMediaType> nativeType;

        hr = _reader->GetNativeMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            typeIndex,
            &nativeType
        );

        if (FAILED(hr))
            break;

        GUID subtype = GUID_NULL;

        UINT32 w = 0;
        UINT32 h = 0;

        UINT32 num = 0;
        UINT32 den = 0;

        nativeType->GetGUID(
            MF_MT_SUBTYPE,
            &subtype
        );

        MFGetAttributeSize(
            nativeType.get(),
            MF_MT_FRAME_SIZE,
            &w,
            &h
        );

        MFGetAttributeRatio(
            nativeType.get(),
            MF_MT_FRAME_RATE,
            &num,
            &den
        );

        WCHAR guidText[64];

        StringFromGUID2(
            subtype,
            guidText,
            ARRAYSIZE(guidText)
        );

        WINTRACE(
            L"[Wcam] NativeType %u -> %s %ux%u %u/%u",
            typeIndex,
            guidText,
            w,
            h,
            num,
            den
        );

        typeIndex++;
    }

    //
    // 원하는 YUY2 찾기
    //
    typeIndex = 0;

    while (true)
    {
        wil::com_ptr_nothrow<IMFMediaType> nativeType;

        hr = _reader->GetNativeMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            typeIndex,
            &nativeType
        );

        if (FAILED(hr))
            break;

        GUID subtype = GUID_NULL;

        UINT32 w = 0;
        UINT32 h = 0;

        UINT32 num = 0;
        UINT32 den = 0;

        nativeType->GetGUID(
            MF_MT_SUBTYPE,
            &subtype
        );

        MFGetAttributeSize(
            nativeType.get(),
            MF_MT_FRAME_SIZE,
            &w,
            &h
        );

        MFGetAttributeRatio(
            nativeType.get(),
            MF_MT_FRAME_RATE,
            &num,
            &den
        );

        if (
            subtype == MFVideoFormat_YUY2 &&
            w == width &&
            h == height
            )
        {
            hr = _reader->SetCurrentMediaType(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                nullptr,
                nativeType.get()
            );

            if (SUCCEEDED(hr))
            {
                WINTRACE(
                    L"[Wcam] Selected YUY2 %ux%u %u/%u",
                    w,
                    h,
                    num,
                    den
                );

                return S_OK;
            }
        }

        typeIndex++;
    }

    //
    // fallback YUY2
    //
    typeIndex = 0;

    while (true)
    {
        wil::com_ptr_nothrow<IMFMediaType> nativeType;

        hr = _reader->GetNativeMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            typeIndex,
            &nativeType
        );

        if (FAILED(hr))
            break;

        GUID subtype = GUID_NULL;

        UINT32 w = 0;
        UINT32 h = 0;

        nativeType->GetGUID(
            MF_MT_SUBTYPE,
            &subtype
        );

        MFGetAttributeSize(
            nativeType.get(),
            MF_MT_FRAME_SIZE,
            &w,
            &h
        );

        if (subtype == MFVideoFormat_YUY2)
        {
            hr = _reader->SetCurrentMediaType(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                nullptr,
                nativeType.get()
            );

            if (SUCCEEDED(hr))
            {
                WINTRACE(
                    L"[Wcam] Selected Fallback YUY2 %ux%u",
                    w,
                    h
                );

                return S_OK;
            }
        }

        typeIndex++;
    }

    WINTRACE(L"[Wcam] No Compatible YUY2 Format");

    return E_FAIL;
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