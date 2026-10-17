#pragma once

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfobjects.h>
#include <wrl/client.h>

class WebcamCapture
{
public:
    WebcamCapture();
    ~WebcamCapture();

    HRESULT Initialize(
        UINT width = 1280,
        UINT height = 960,
        UINT fps = 30
    );

    HRESULT GetFrame(IMFSample** sample);

    HRESULT GetOutputType(IMFMediaType** type)
    {
        if (!type) return E_POINTER;
        *type = nullptr;
        if (!_reader) return E_UNEXPECTED;
        return _reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, type);
    }

    void Shutdown();

private:
    Microsoft::WRL::ComPtr<IMFMediaSource> _source;
    Microsoft::WRL::ComPtr<IMFSourceReader> _reader;

    UINT _width = 0;
    UINT _height = 0;
    UINT _fps = 0;
};