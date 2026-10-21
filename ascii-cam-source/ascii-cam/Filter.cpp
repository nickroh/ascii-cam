#include "pch.h"
#include "Filter.h"

HRESULT Filter::Initialize(UINT width, UINT height, FilterOption option)
{
    Shutdown();

    if (!width || !height)
        return E_INVALIDARG;

    _width = width;
    _height = height;

    return SetFilterOption(option);
}

HRESULT Filter::SetFilterOption(FilterOption option)
{
    if (!_width || !_height)
        return MF_E_NOT_INITIALIZED;

    switch (option)
    {
    case FilterOption::ASCII:
        if (!_asciiReady)
        {
            RETURN_IF_FAILED(_ascii.Initialize(_width, _height));
            _asciiReady = true;
        }
        break;

    case FilterOption::Grayscale:
        if (!_grayReady)
        {
            RETURN_IF_FAILED(_grayscale.Initialize(_width, _height));
            _grayReady = true;
        }
        break;

    case FilterOption::Passthrough:
        break;

    default:
        return E_NOTIMPL;
    }

    _option = option;
    return S_OK;
}

HRESULT Filter::Process(IMFSample* sample)
{
    RETURN_HR_IF_NULL(E_POINTER, sample);

    if (_option == FilterOption::Passthrough)
        return S_OK;

    if (_option == FilterOption::Grayscale)
        return _grayscale.Process(sample);

    if (_option != FilterOption::ASCII)
        return E_NOTIMPL;

    wil::com_ptr_nothrow<IMFMediaBuffer> buffer;
    RETURN_IF_FAILED(sample->GetBufferByIndex(0, &buffer));

    wil::com_ptr_nothrow<IMF2DBuffer> buffer2D;

    if (SUCCEEDED(buffer->QueryInterface(IID_PPV_ARGS(&buffer2D))))
    {
        BYTE* data = nullptr;
        LONG pitch = 0;

        RETURN_IF_FAILED(buffer2D->Lock2D(&data, &pitch));
        HRESULT hr = _ascii.Process(data, pitch);
        HRESULT unlockHr = buffer2D->Unlock2D();

        return FAILED(hr) ? hr : unlockHr;
    }

    BYTE* data = nullptr;
    DWORD maxLength = 0, currentLength = 0;

    RETURN_IF_FAILED(buffer->Lock(&data, &maxLength, &currentLength));

    HRESULT hr = _ascii.Process(data, static_cast<LONG>(_width));
    HRESULT unlockHr = buffer->Unlock();

    return FAILED(hr) ? hr : unlockHr;
}

void Filter::Shutdown()
{
    _ascii.Shutdown();
    _grayscale.Shutdown();

    _asciiReady = false;
    _grayReady = false;
    _option = FilterOption::Passthrough;
    _width = _height = 0;
}