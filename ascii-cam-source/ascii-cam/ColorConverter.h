#pragma once

#include <mfapi.h>
#include <mfidl.h>
#include <mftransform.h>
#include <mfobjects.h>
#include <mferror.h>
#include <wrl/client.h>

class ColorConverter
{
public:
    ColorConverter();
    ~ColorConverter();

    HRESULT Initialize(
        UINT width,
        UINT height
    );

    HRESULT Convert(
        IMFSample* inputSample,
        IMFSample** outputSample
    );

    void Shutdown();

private:
    Microsoft::WRL::ComPtr<IMFTransform> _transform;

    UINT _width = 0;
    UINT _height = 0;
};