#include "pch.h"
#include "GrayscaleEngine.h"
#include "ShaderPath.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstring>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

HRESULT GrayscaleEngine::Initialize(UINT width, UINT height)
{
    Shutdown();

    if (!width || !height || (width % 2) || (height % 2))
        return E_INVALIDARG;

    _width = width;
    _height = height;

    wil::com_ptr_nothrow<ID3D11Device> device;
    wil::com_ptr_nothrow<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL featureLevel{};

    RETURN_IF_FAILED(D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &device, &featureLevel, &context
    ));

    wil::com_ptr_nothrow<ID3DBlob> blob;
    wil::com_ptr_nothrow<ID3DBlob> errors;

    auto path = GetShaderPath(L"GrayscaleShader.hlsl");

    RETURN_IF_FAILED(D3DCompileFromFile(
        path.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main", "cs_5_0", 0, 0, &blob, &errors
    ));

    wil::com_ptr_nothrow<ID3D11ComputeShader> shader;

    RETURN_IF_FAILED(device->CreateComputeShader(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        nullptr, &shader
    ));

    // One RG8 pixel stores a U/V pair.
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width / 2;
    desc.Height = height / 2;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    wil::com_ptr_nothrow<ID3D11Texture2D> texture;
    wil::com_ptr_nothrow<ID3D11UnorderedAccessView> uav;

    RETURN_IF_FAILED(device->CreateTexture2D(&desc, nullptr, &texture));
    RETURN_IF_FAILED(device->CreateUnorderedAccessView(texture.get(), nullptr, &uav));

    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    wil::com_ptr_nothrow<ID3D11Texture2D> staging;
    RETURN_IF_FAILED(device->CreateTexture2D(&desc, nullptr, &staging));

    context->CSSetShader(shader.get(), nullptr, 0);

    ID3D11UnorderedAccessView* views[] = { uav.get() };
    context->CSSetUnorderedAccessViews(0, 1, views, nullptr);

    context->Dispatch(
        (desc.Width + 7) / 8,
        (desc.Height + 7) / 8,
        1
    );

    ID3D11UnorderedAccessView* nullView = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, &nullView, nullptr);

    context->CopyResource(staging.get(), texture.get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    RETURN_IF_FAILED(context->Map(
        staging.get(), 0, D3D11_MAP_READ, 0, &mapped
    ));

    _neutralUV.resize(static_cast<size_t>(width) * height / 2);

    for (UINT y = 0; y < height / 2; ++y)
    {
        std::memcpy(
            _neutralUV.data() + static_cast<size_t>(y) * width,
            static_cast<const BYTE*>(mapped.pData) + static_cast<size_t>(y) * mapped.RowPitch,
            width
        );
    }

    context->Unmap(staging.get(), 0);
    return S_OK;
}

HRESULT GrayscaleEngine::Process(IMFSample* sample)
{
    if (!sample)
        return E_POINTER;

    if (_neutralUV.empty())
        return MF_E_NOT_INITIALIZED;

    wil::com_ptr_nothrow<IMFMediaBuffer> buffer;
    RETURN_IF_FAILED(sample->ConvertToContiguousBuffer(&buffer));

    BYTE* data = nullptr;
    DWORD maxLength = 0;
    DWORD length = 0;

    RETURN_IF_FAILED(buffer->Lock(&data, &maxLength, &length));

    const size_t ySize = static_cast<size_t>(_width) * _height;
    const size_t required = ySize + _neutralUV.size();

    HRESULT hr = S_OK;

    if (length < required)
        hr = MF_E_BUFFERTOOSMALL;
    else
        std::memcpy(data + ySize, _neutralUV.data(), _neutralUV.size());

    HRESULT unlockHr = buffer->Unlock();
    return FAILED(hr) ? hr : unlockHr;
}

void GrayscaleEngine::Shutdown()
{
    _neutralUV.clear();
    _width = _height = 0;
}