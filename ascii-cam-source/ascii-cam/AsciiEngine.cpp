#include "pch.h"
#include "AsciiEngine.h"
#include "ShaderPath.h"

#include <d3dcompiler.h>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

AsciiEngine::~AsciiEngine()
{
    Shutdown();
}

HRESULT AsciiEngine::Initialize(UINT width, UINT height)
{
    Shutdown();

    _width = width;
    _height = height;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0
    };

    D3D_FEATURE_LEVEL outFeatureLevel{};

    RETURN_IF_FAILED(
        D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createDeviceFlags,
            featureLevels,
            _countof(featureLevels),
            D3D11_SDK_VERSION,
            &_device,
            &outFeatureLevel,
            &_context
        )
    );

    // Load external ASCII shader.
    wil::com_ptr_nothrow<ID3DBlob> shaderBlob;
    wil::com_ptr_nothrow<ID3DBlob> errorBlob;

    std::wstring shaderPath = GetShaderPath(L"AsciiShader.hlsl");

    if (shaderPath.empty())
        return E_FAIL;

    HRESULT hr = D3DCompileFromFile(
        shaderPath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        "cs_5_0",
        0,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA(
                static_cast<const char*>(errorBlob->GetBufferPointer())
            );
        }

        return hr;
    }

    RETURN_IF_FAILED(
        _device->CreateComputeShader(
            shaderBlob->GetBufferPointer(),
            shaderBlob->GetBufferSize(),
            nullptr,
            &_computeShader
        )
    );

    // Input texture: Y plane
    D3D11_TEXTURE2D_DESC inputDesc{};
    inputDesc.Width = _width;
    inputDesc.Height = _height;
    inputDesc.MipLevels = 1;
    inputDesc.ArraySize = 1;
    inputDesc.Format = DXGI_FORMAT_R8_UNORM;
    inputDesc.SampleDesc.Count = 1;
    inputDesc.Usage = D3D11_USAGE_DYNAMIC;
    inputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    inputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    RETURN_IF_FAILED(
        _device->CreateTexture2D(
            &inputDesc,
            nullptr,
            &_inputTexture
        )
    );

    RETURN_IF_FAILED(
        _device->CreateShaderResourceView(
            _inputTexture.get(),
            nullptr,
            &_inputSRV
        )
    );

    // Shader output texture
    D3D11_TEXTURE2D_DESC outputDesc{};
    outputDesc.Width = _width;
    outputDesc.Height = _height;
    outputDesc.MipLevels = 1;
    outputDesc.ArraySize = 1;
    outputDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    outputDesc.SampleDesc.Count = 1;
    outputDesc.Usage = D3D11_USAGE_DEFAULT;
    outputDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    RETURN_IF_FAILED(
        _device->CreateTexture2D(
            &outputDesc,
            nullptr,
            &_outputTexture
        )
    );

    RETURN_IF_FAILED(
        _device->CreateUnorderedAccessView(
            _outputTexture.get(),
            nullptr,
            &_outputUAV
        )
    );

    // GPU-to-CPU staging texture
    D3D11_TEXTURE2D_DESC stagingDesc{};
    stagingDesc.Width = _width;
    stagingDesc.Height = _height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    RETURN_IF_FAILED(
        _device->CreateTexture2D(
            &stagingDesc,
            nullptr,
            &_stagingTexture
        )
    );

    return S_OK;
}

HRESULT AsciiEngine::Process(const BYTE* yPlane, LONG pitch)
{
    if (!_inputTexture ||
        !_inputSRV ||
        !_outputTexture ||
        !_outputUAV ||
        !_stagingTexture ||
        !_context ||
        !_computeShader ||
        !yPlane)
    {
        return E_FAIL;
    }

    // Upload Y plane to GPU.
    D3D11_MAPPED_SUBRESOURCE mappedInput{};

    RETURN_IF_FAILED(
        _context->Map(
            _inputTexture.get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mappedInput
        )
    );

    const BYTE* srcRow = yPlane;
    BYTE* dstRow = static_cast<BYTE*>(mappedInput.pData);

    for (UINT y = 0; y < _height; ++y)
    {
        memcpy(dstRow, srcRow, _width);
        srcRow += pitch;
        dstRow += mappedInput.RowPitch;
    }

    _context->Unmap(_inputTexture.get(), 0);

    // Execute ASCII compute shader.
    _context->CSSetShader(
        _computeShader.get(),
        nullptr,
        0
    );

    ID3D11ShaderResourceView* srvs[] =
    {
        _inputSRV.get()
    };

    _context->CSSetShaderResources(0, 1, srvs);

    ID3D11UnorderedAccessView* uavs[] =
    {
        _outputUAV.get()
    };

    _context->CSSetUnorderedAccessViews(
        0,
        1,
        uavs,
        nullptr
    );

    _context->Dispatch(
        (_width + 7) / 8,
        (_height + 7) / 8,
        1
    );

    // Unbind shader resources.
    ID3D11ShaderResourceView* nullSRV = nullptr;
    _context->CSSetShaderResources(0, 1, &nullSRV);

    ID3D11UnorderedAccessView* nullUAV = nullptr;
    _context->CSSetUnorderedAccessViews(
        0,
        1,
        &nullUAV,
        nullptr
    );

    // Copy GPU output to CPU-readable texture.
    _context->CopyResource(
        _stagingTexture.get(),
        _outputTexture.get()
    );

    D3D11_MAPPED_SUBRESOURCE mappedStaging{};

    RETURN_IF_FAILED(
        _context->Map(
            _stagingTexture.get(),
            0,
            D3D11_MAP_READ,
            0,
            &mappedStaging
        )
    );

    const BYTE* srcResultRow =
        static_cast<const BYTE*>(mappedStaging.pData);

    BYTE* dstResultRow = const_cast<BYTE*>(yPlane);

    // Write ASCII luminance back into the Y plane.
    for (UINT y = 0; y < _height; ++y)
    {
        const BYTE* srcPixel = srcResultRow;
        BYTE* dstPixel = dstResultRow;

        for (UINT x = 0; x < _width; ++x)
        {
            dstPixel[x] = srcPixel[x * 4];
        }

        srcResultRow += mappedStaging.RowPitch;
        dstResultRow += pitch;
    }

    _context->Unmap(_stagingTexture.get(), 0);

    return S_OK;
}

void AsciiEngine::Shutdown()
{
    _computeShader.reset();
    _stagingTexture.reset();
    _outputUAV.reset();
    _outputTexture.reset();
    _inputSRV.reset();
    _inputTexture.reset();
    _context.reset();
    _device.reset();
}