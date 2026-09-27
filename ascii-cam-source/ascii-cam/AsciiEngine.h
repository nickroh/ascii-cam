#pragma once

#include <windows.h>
#include <d3d11.h>
#include <wil/com.h>
#include <wil/result.h>
#include <cstdint>

class AsciiEngine
{
public:
    AsciiEngine() = default;
    ~AsciiEngine();

    // 초기화: D3D11 디바이스, 입력/출력 텍스처, 컴퓨트 셰이더 컴파일 및 생성
    HRESULT Initialize(UINT width, UINT height);

    // 프로세스: CPU 데이터를 GPU로 업로드 -> 컴퓨트 셰이더 실행 -> 결과를 다시 CPU로 다운로드
    HRESULT Process(const BYTE* yPlane, LONG pitch);

    // 자원 해제
    void Shutdown();

private:
    UINT _width = 0;
    UINT _height = 0;

    // Direct3D 11 VRAM 자원
    wil::com_ptr_nothrow<ID3D11Device> _device;
    wil::com_ptr_nothrow<ID3D11DeviceContext> _context;
    wil::com_ptr_nothrow<ID3D11Texture2D> _inputTexture;         // CPU 입력용 동적 텍스처
    wil::com_ptr_nothrow<ID3D11ShaderResourceView> _inputSRV;    // 셰이더 입력 SRV
    wil::com_ptr_nothrow<ID3D11Texture2D> _outputTexture;        // 셰이더 출력 텍스처 (UAV 연동)
    wil::com_ptr_nothrow<ID3D11UnorderedAccessView> _outputUAV;  // 셰이더 출력 UAV
    wil::com_ptr_nothrow<ID3D11Texture2D> _stagingTexture;       // GPU -> CPU 다운로드용 스테이징 텍스처
    wil::com_ptr_nothrow<ID3D11ComputeShader> _computeShader;    // 아스키 변환 컴퓨트 셰이더

    // 기존 아스키 상수 유지
    static constexpr UINT CELL_WIDTH = 8;
    static constexpr UINT CELL_HEIGHT = 12;
    static constexpr UINT BLOCK_SIZE = 8;
    static constexpr UINT LEVEL_COUNT = 9;
    static constexpr BYTE LEVELS[LEVEL_COUNT] = { 0, 32, 64, 96, 128, 160, 192, 224, 255 };
    static constexpr char CHARSET[] = "@#W$9876543210?!abc;:+=-,._ ";
};