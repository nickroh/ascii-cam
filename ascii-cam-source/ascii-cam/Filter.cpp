#include "pch.h"
#include "Filter.h"
#include <vector>
#include <cstring>

Filter::Filter()
{
}

Filter::~Filter()
{
    Shutdown();
}

HRESULT Filter::Initialize(UINT width, UINT height, FilterOption option, IMFMediaType* inputType)
{
    _width = width;
    _height = height;
    _option = option;
    if (_option == FilterOption::Grayscale)
    {
        RETURN_HR_IF_NULL(E_POINTER, inputType);
        RETURN_IF_FAILED(inputType->GetGUID(MF_MT_SUBTYPE, &_inputFormat));
        RETURN_IF_FAILED(MFGetAttributeSize(inputType, MF_MT_FRAME_SIZE, &_width, &_height));
        RETURN_HR_IF(MF_E_INVALIDMEDIATYPE,
            _inputFormat != MFVideoFormat_NV12 && _inputFormat != MFVideoFormat_YUY2);
        RETURN_HR_IF(MF_E_INVALIDMEDIATYPE, !_width || !_height || (_width % 2) ||
            (_inputFormat == MFVideoFormat_NV12 && (_height % 2)) || _width > MAXLONG / 2);
        const UINT rowBytes = _inputFormat == MFVideoFormat_NV12 ? _width : _width * 2;
        _stride = MFGetAttributeUINT32(inputType, MF_MT_DEFAULT_STRIDE, rowBytes);
        RETURN_HR_IF(MF_E_INVALIDMEDIATYPE, _stride < rowBytes || _stride > MAXLONG);
    }

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

    if (_option == FilterOption::Grayscale)
    {
        // Normalize 2D/GPU-backed buffers to packed rows, preserving their layout on writeback.
        wil::com_ptr_nothrow<IMF2DBuffer> image;
        if (SUCCEEDED(buffer->QueryInterface(IID_PPV_ARGS(&image))))
        {
            DWORD length = 0;
            RETURN_IF_FAILED(image->GetContiguousLength(&length));
            std::vector<BYTE> pixels(length);
            RETURN_IF_FAILED(image->ContiguousCopyTo(pixels.data(), length));
            const UINT stride = _inputFormat == MFVideoFormat_NV12 ? _width : _width * 2;
            RETURN_IF_FAILED(ProcessGrayscale(pixels.data(), length, stride));
            return image->ContiguousCopyFrom(pixels.data(), length);
        }

        BYTE* data = nullptr;
        DWORD length = 0;
        RETURN_IF_FAILED(buffer->Lock(&data, nullptr, &length));
        const HRESULT result = ProcessGrayscale(data, length, _stride);
        const HRESULT unlockResult = buffer->Unlock();
        return FAILED(result) ? result : unlockResult;
    }

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

HRESULT Filter::ProcessGrayscale(BYTE* data, DWORD length, UINT stride) const
{
    RETURN_HR_IF_NULL(E_POINTER, data);
    if (_inputFormat == MFVideoFormat_NV12)
    {
        // Keep brightness (Y); set interleaved color (U/V) to neutral.
        const uint64_t yBytes = static_cast<uint64_t>(stride) * _height;
        const uint64_t required = yBytes + static_cast<uint64_t>(stride) * (_height / 2);
        RETURN_HR_IF(MF_E_BUFFERTOOSMALL, required > length);
        BYTE* uv = data + static_cast<size_t>(yBytes);
        for (UINT row = 0; row < _height / 2; ++row)
            std::memset(uv + static_cast<size_t>(row) * stride, 128, _width);
        return S_OK;
    }
    if (_inputFormat == MFVideoFormat_YUY2)
    {
        // Packed Y0 U Y1 V: only odd bytes contain color.
        RETURN_HR_IF(MF_E_BUFFERTOOSMALL, static_cast<uint64_t>(stride) * _height > length);
        for (UINT row = 0; row < _height; ++row)
        {
            BYTE* pixels = data + static_cast<size_t>(row) * stride;
            for (UINT column = 1; column < _width * 2; column += 2)
                pixels[column] = 128;
        }
        return S_OK;
    }
    return MF_E_INVALIDMEDIATYPE;
}

void Filter::Shutdown()
{
}