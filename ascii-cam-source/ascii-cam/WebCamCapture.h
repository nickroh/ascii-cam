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

    void Shutdown();

private:
    Microsoft::WRL::ComPtr<IMFMediaSource> _source;
    Microsoft::WRL::ComPtr<IMFSourceReader> _reader;

    UINT _width = 0;
    UINT _height = 0;
    UINT _fps = 0;
};