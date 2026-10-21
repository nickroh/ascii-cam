#include "pch.h"
#include "Filter.h"

Filter::Filter()
{
}

Filter::~Filter()
{
    Shutdown();
}

HRESULT Filter::Initialize(UINT width, UINT height, FilterOption option)
{
    _width = width;
    _height = height;

    // Engines are sized for the previous resolution; recreate them on demand.
    _asciiReady = false;
    _grayscaleReady = false;

    RETURN_IF_FAILED(EnsureEngine(option));
    _option = option;

    return S_OK;
}

HRESULT Filter::SetOption(FilterOption option)
{
    if (option == _option)
        return S_OK;

    RETURN_IF_FAILED(EnsureEngine(option));
    _option = option;

    return S_OK;
}

HRESULT Filter::EnsureEngine(FilterOption option)
{
    if (option == FilterOption::Grayscale && !_grayscaleReady)
    {
        RETURN_IF_FAILED(_grayscale.Initialize(_width, _height));
        _grayscaleReady = true;
    }

    if (option == FilterOption::ASCII && !_asciiReady)
    {
        RETURN_IF_FAILED(_ascii.Initialize(_width, _height));
        _asciiReady = true;
    }

    return S_OK;
}

HRESULT Filter::Process(IMFSample* sample)
{
    RETURN_HR_IF_NULL(E_POINTER, sample);

    if (_option == FilterOption::Passthrough)
        return S_OK;

    wil::com_ptr_nothrow<IMFMediaBuffer> buffer;
    RETURN_IF_FAILED(sample->GetBufferByIndex(0, &buffer));

    wil::com_ptr_nothrow<IMF2DBuffer> buffer2D;

    if (SUCCEEDED(buffer->QueryInterface(IID_PPV_ARGS(&buffer2D))))
    {
        BYTE* data = nullptr;
        LONG pitch = 0;

        RETURN_IF_FAILED(buffer2D->Lock2D(&data, &pitch));

        HRESULT hr = S_OK;

        if (_option == FilterOption::ASCII)
        {
            hr = _ascii.Process(data, pitch);
        }
        else if (_option == FilterOption::Grayscale)
        {
            hr = _grayscale.Process(data, pitch);
        }

        HRESULT unlockHr = buffer2D->Unlock2D();

        RETURN_IF_FAILED(hr);
        RETURN_IF_FAILED(unlockHr);

        return S_OK;
    }

    BYTE* data = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;

    RETURN_IF_FAILED(buffer->Lock(&data, &maxLength, &currentLength));

    HRESULT hr = S_OK;

    if (_option == FilterOption::ASCII)
    {
        hr = _ascii.Process(data, static_cast<LONG>(_width));
    }
    else if (_option == FilterOption::Grayscale)
    {
        hr = _grayscale.Process(data, static_cast<LONG>(_width));
    }

    HRESULT unlockHr = buffer->Unlock();

    RETURN_IF_FAILED(hr);
    RETURN_IF_FAILED(unlockHr);

    return S_OK;
}

void Filter::Shutdown()
{
    _ascii.Shutdown();
    _grayscale.Shutdown();
    _asciiReady = false;
    _grayscaleReady = false;
}