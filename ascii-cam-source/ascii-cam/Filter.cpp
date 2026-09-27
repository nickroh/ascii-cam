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
    _option = option;

    if (_option == FilterOption::ASCII)
    {
        // AsciiEngine::Initialize returns void.
        _ascii.Initialize(_width, _height);
    } 
    if (_option == FilterOption::Passthrough) 
    {

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

        if (_option == FilterOption::ASCII)
        {
            // Keep pitch signed; IMF2DBuffer may use a negative pitch.
            _ascii.Process(data, pitch);
        }

        buffer2D->Unlock2D();
        return S_OK;
    }

    BYTE* data = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;

    RETURN_IF_FAILED(buffer->Lock(&data, &maxLength, &currentLength));

    if (_option == FilterOption::ASCII)
    {
        _ascii.Process(data, static_cast<LONG>(_width));
    }

    buffer->Unlock();
    return S_OK;
}

void Filter::Shutdown()
{
}