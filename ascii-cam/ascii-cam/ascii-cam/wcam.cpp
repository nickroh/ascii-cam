#include "pch.h"
#include "wcam.h"

#include <mferror.h>

using Microsoft::WRL::ComPtr;

WebcamCapture::WebcamCapture()
{
}

WebcamCapture::~WebcamCapture()
{
    Shutdown();
}

HRESULT WebcamCapture::Initialize(UINT width, UINT height, UINT fps)
{
    _width = width;
    _height = height;
    _fps = fps;

    HRESULT hr = S_OK;

    ComPtr<IMFAttributes> attributes;

    hr = MFCreateAttributes(&attributes, 1);
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
        attributes.Get(),
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

    // 첫 번째 webcam 사용
    hr = devices[0]->ActivateObject(
        IID_PPV_ARGS(&_source)
    );

    for (UINT32 i = 0; i < count; i++)
    {
        devices[i]->Release();
    }

    CoTaskMemFree(devices);

    if (FAILED(hr))
        return hr;

    hr = MFCreateSourceReaderFromMediaSource(
        _source.Get(),
        nullptr,
        &_reader
    );

    if (FAILED(hr))
        return hr;

    //
    // NV12 format 설정
    //
    ComPtr<IMFMediaType> mediaType;

    hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr))
        return hr;

    hr = mediaType->SetGUID(
        MF_MT_MAJOR_TYPE,
        MFMediaType_Video
    );

    if (FAILED(hr))
        return hr;

    hr = mediaType->SetGUID(
        MF_MT_SUBTYPE,
        MFVideoFormat_NV12
    );

    if (FAILED(hr))
        return hr;

    hr = MFSetAttributeSize(
        mediaType.Get(),
        MF_MT_FRAME_SIZE,
        width,
        height
    );

    if (FAILED(hr))
        return hr;

    hr = MFSetAttributeRatio(
        mediaType.Get(),
        MF_MT_FRAME_RATE,
        fps,
        1
    );

    if (FAILED(hr))
        return hr;

    hr = _reader->SetCurrentMediaType(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        nullptr,
        mediaType.Get()
    );

    if (FAILED(hr))
        return hr;

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