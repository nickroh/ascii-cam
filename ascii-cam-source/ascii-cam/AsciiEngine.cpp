#include "pch.h"
#include "AsciiEngine.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

static const char* g_AsciiComputeShaderHLSL = R"(
Texture2D<float> inputTex : register(t0);
RWTexture2D<float4> outputTex : register(u0);

static const uint CHAR_WIDTH = 8;
static const uint CHAR_HEIGHT = 16;
static const uint CHAR_COUNT = 10;

static const uint glyphs[CHAR_COUNT][CHAR_HEIGHT] =
{
    { 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 },                               // space
    { 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0x18,0x18 },                         // .
    { 0,0,0,0,0,0,0,0, 0x18,0x18,0,0,0,0x18,0x18,0 },                   // :
    { 0,0,0,0,0,0,0,0, 0,0x7E,0x7E,0,0,0,0,0 },                         // -
    { 0,0,0,0,0,0,0x7E,0x7E, 0,0,0x7E,0x7E,0,0,0,0 },                   // =
    { 0,0,0,0,0,0x18,0x18,0x18, 0x7E,0x7E,0x18,0x18,0x18,0,0,0 },       // +
    { 0,0,0,0,0,0x18,0x18,0x5A, 0x3C,0x3C,0x5A,0x18,0x18,0,0,0 },       // *
    { 0,0,0,0x24,0x24,0x7E,0x7E,0x24, 0x24,0x7E,0x7E,0x24,0x24,0,0,0 }, // #
    { 0,0,0,0x62,0x62,0x64,0x08,0x10, 0x20,0x4C,0x8C,0x8C,0,0,0,0 },     // %
    { 0,0,0,0x3C,0x42,0x99,0xA5,0xA5, 0x9D,0x40,0x3C,0x3C,0,0,0,0 }      // @
};

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint width, height;
    inputTex.GetDimensions(width, height);

    uint2 pos = dtid.xy;
    if (pos.x >= width || pos.y >= height) return;

    uint2 cellStart = (pos / uint2(CHAR_WIDTH, CHAR_HEIGHT)) * uint2(CHAR_WIDTH, CHAR_HEIGHT);
    uint2 cellEnd = min(cellStart + uint2(CHAR_WIDTH, CHAR_HEIGHT), uint2(width, height));

    float lumSum = 0.0f;
    uint count = 0;
    
    for (uint y = cellStart.y; y < cellEnd.y; ++y)
    {
        for (uint x = cellStart.x; x < cellEnd.x; ++x)
        {
            lumSum += inputTex.Load(int3(x, y, 0)); 
            ++count;
        }
    }

    float luminance = lumSum / max((float)count, 1.0f);
    float tone = pow(saturate(luminance), 0.82f);
    uint glyphIndex = min((uint)floor(tone * (CHAR_COUNT - 1) + 0.5f), CHAR_COUNT - 1);

    uint2 local = pos - cellStart;
    uint rowBits = glyphs[glyphIndex][local.y];
    bool ink = ((rowBits >> (7 - local.x)) & 1) != 0;

    outputTex[pos] = ink ? float4(1.0f, 1.0f, 1.0f, 1.0f) : float4(0.0f, 0.0f, 0.0f, 1.0f);
}
)";

AsciiEngine::~AsciiEngine()
{
    Shutdown();
}

HRESULT AsciiEngine::Initialize(UINT width, UINT height)
{
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

    D3D_FEATURE_LEVEL outFeatureLevel;

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

    wil::com_ptr_nothrow<ID3DBlob> shaderBlob;
    wil::com_ptr_nothrow<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompile(
        g_AsciiComputeShaderHLSL,
        strlen(g_AsciiComputeShaderHLSL),
        "AsciiCS",
        nullptr,
        nullptr,
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
                static_cast<const char*>(
                    errorBlob->GetBufferPointer()
                    )
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

    D3D11_TEXTURE2D_DESC inputDesc = {};
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

    D3D11_TEXTURE2D_DESC outputDesc = {};
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

    D3D11_TEXTURE2D_DESC stagingDesc = {};
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

HRESULT AsciiEngine::Process(
    const BYTE* yPlane,
    LONG pitch
)
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

    D3D11_MAPPED_SUBRESOURCE mappedInput = {};

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

    _context->Unmap(
        _inputTexture.get(),
        0
    );

    _context->CSSetShader(
        _computeShader.get(),
        nullptr,
        0
    );

    ID3D11ShaderResourceView* srvs[] =
    {
        _inputSRV.get()
    };

    _context->CSSetShaderResources(
        0,
        1,
        srvs
    );

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

    const UINT dispatchX = (_width + 7) / 8;
    const UINT dispatchY = (_height + 7) / 8;

    _context->Dispatch(
        dispatchX,
        dispatchY,
        1
    );

    ID3D11ShaderResourceView* nullSRV = nullptr;
    _context->CSSetShaderResources(0, 1, &nullSRV);

    ID3D11UnorderedAccessView* nullUAV = nullptr;
    _context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

    _context->CopyResource(
        _stagingTexture.get(),
        _outputTexture.get()
    );

    D3D11_MAPPED_SUBRESOURCE mappedStaging = {};

    RETURN_IF_FAILED(
        _context->Map(
            _stagingTexture.get(),
            0,
            D3D11_MAP_READ,
            0,
            &mappedStaging
        )
    );

    const BYTE* srcResultRow = static_cast<const BYTE*>(mappedStaging.pData);
    BYTE* dstResultRow = const_cast<BYTE*>(yPlane);

    for (UINT y = 0; y < _height; ++y)
    {
        const BYTE* srcPixel = srcResultRow;
        BYTE* dstPixel = dstResultRow;

        for (UINT x = 0; x < _width; ++x)
        {
            dstPixel[x] = srcPixel[x * 4 + 0];
        }

        srcResultRow += mappedStaging.RowPitch;
        dstResultRow += pitch;
    }

    _context->Unmap(
        _stagingTexture.get(),
        0
    );

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