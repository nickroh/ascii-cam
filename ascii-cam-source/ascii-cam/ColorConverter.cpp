#include "ColorConverter.h"

#include <wmcodecdsp.h>

ColorConverter::ColorConverter()
{
}

ColorConverter::~ColorConverter()
{
    Shutdown();
}

HRESULT ColorConverter::Initialize(
    UINT width,
    UINT height
)
{
    _width = width;
    _height = height;

    HRESULT hr = CoCreateInstance(
        CLSID_CColorConvertDMO,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&_transform)
    );

    if (FAILED(hr))
        return hr;

    //
    // INPUT TYPE (YUY2)
    //
    Microsoft::WRL::ComPtr<IMFMediaType> inputType;

    hr = MFCreateMediaType(&inputType);

    if (FAILED(hr))
        return hr;

    hr = inputType->SetGUID(
        MF_MT_MAJOR_TYPE,
        MFMediaType_Video
    );

    if (FAILED(hr))
        return hr;

    hr = inputType->SetGUID(
        MF_MT_SUBTYPE,
        MFVideoFormat_YUY2
    );

    if (FAILED(hr))
        return hr;

    hr = MFSetAttributeSize(
        inputType.Get(),
        MF_MT_FRAME_SIZE,
        width,
        height
    );

    if (FAILED(hr))
        return hr;

    hr = MFSetAttributeRatio(
        inputType.Get(),
        MF_MT_FRAME_RATE,
        30,
        1
    );

    if (FAILED(hr))
        return hr;

    hr = _transform->SetInputType(
        0,
        inputType.Get(),
        0
    );

    if (FAILED(hr))
        return hr;

    //
    // OUTPUT TYPE (NV12)
    //
    Microsoft::WRL::ComPtr<IMFMediaType> outputType;

    hr = MFCreateMediaType(&outputType);

    if (FAILED(hr))
        return hr;

    hr = outputType->SetGUID(
        MF_MT_MAJOR_TYPE,
        MFMediaType_Video
    );

    if (FAILED(hr))
        return hr;

    hr = outputType->SetGUID(
        MF_MT_SUBTYPE,
        MFVideoFormat_NV12
    );

    if (FAILED(hr))
        return hr;

    hr = MFSetAttributeSize(
        outputType.Get(),
        MF_MT_FRAME_SIZE,
        width,
        height
    );

    if (FAILED(hr))
        return hr;

    hr = MFSetAttributeRatio(
        outputType.Get(),
        MF_MT_FRAME_RATE,
        30,
        1
    );

    if (FAILED(hr))
        return hr;

    hr = _transform->SetOutputType(
        0,
        outputType.Get(),
        0
    );

    if (FAILED(hr))
        return hr;

    return S_OK;
}

HRESULT ColorConverter::Convert(
    IMFSample* inputSample,
    IMFSample** outputSample
)
{
    if (!inputSample || !outputSample)
        return E_POINTER;

    *outputSample = nullptr;

    HRESULT hr = _transform->ProcessInput(
        0,
        inputSample,
        0
    );

    if (FAILED(hr))
        return hr;

    //
    // output sample »ý¼º
    //
    Microsoft::WRL::ComPtr<IMFSample> sample;

    hr = MFCreateSample(&sample);

    if (FAILED(hr))
        return hr;

    DWORD bufferSize =
        (_width * _height * 3) / 2;

    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;

    hr = MFCreateMemoryBuffer(
        bufferSize,
        &buffer
    );

    if (FAILED(hr))
        return hr;

    hr = sample->AddBuffer(buffer.Get());

    if (FAILED(hr))
        return hr;

    MFT_OUTPUT_DATA_BUFFER outputData = {};

    outputData.dwStreamID = 0;
    outputData.pSample = sample.Get();

    DWORD status = 0;

    hr = _transform->ProcessOutput(
        0,
        1,
        &outputData,
        &status
    );

    if (SUCCEEDED(hr))
    {
        Microsoft::WRL::ComPtr<IMFMediaBuffer> outBuffer;

        hr = sample->GetBufferByIndex(
            0,
            &outBuffer
        );

        if (FAILED(hr))
            return hr;

        DWORD currentLength =
            (_width * _height * 3) / 2;

        hr = outBuffer->SetCurrentLength(
            currentLength
        );

        if (FAILED(hr))
            return hr;
    }

    if (FAILED(hr))
        return hr;

    *outputSample = sample.Detach();

    return S_OK;
}

void ColorConverter::Shutdown()
{
    _transform.Reset();
}