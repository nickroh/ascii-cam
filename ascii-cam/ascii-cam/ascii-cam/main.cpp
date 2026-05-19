//#include <iostream>
//#include <opencv2/opencv.hpp>
//#include "AsciiEngine.h"
//#include "VirtualCam.h"
//#include <conio.h>
//
//int test_ascii() {
//    // 1. Load the image file
//    // Replace "test.jpg" with your actual file path. 
//    // Supports jpg, png, bmp, etc.
//    std::string filePath = "max.jpg";
//    cv::Mat image = cv::imread(filePath);
//
//    if (image.empty()) {
//        std::cerr << "Error: Could not open or find the image!" << std::endl;
//        return -1;
//    }
//
//    // 2. Initialize the AsciiEngine
//    // Setting character size to 8x12 pixels. 
//    // Smaller numbers = Higher detail (more characters)
//    AsciiEngine engine(8, 12);
//
//    // 3. Optional: Resize if the image is too large for your monitor
//    if (image.cols > 1280) {
//        double scale = 1280.0 / image.cols;
//        cv::resize(image, image, cv::Size(), scale, scale);
//    }
//
//    // 4. Process the image
//    cv::Mat asciiImage = engine.Process(image);
//
//    // 5. Display the results
//    cv::imshow("Original Image", image);
//    cv::imshow("ASCII Art Output", asciiImage);
//
//    // 6. Optional: Save the result to a file
//    // cv::imwrite("ascii_result.png", asciiImage);
//
//    std::cout << "Press any key to exit..." << std::endl;
//    cv::waitKey(0); // Wait for a keyboard stroke
//
//    return 0;
//}
//
//int test_cam() {
//    VirtualCam vc(L"MY CAM"); // friendlyName이 wstring이므로 L 붙이기
//
//    if (vc.Initialize()) {
//        std::wcout << L"카메라가 켜졌습니다. 아무 키나 누르면 종료합니다..." << std::endl;
//
//        // 카메라가 장치 관리자에 떠 있는 동안 프로그램을 대기시킵니다.
//        _getch();
//    }
//    else {
//        std::wcerr << L"카메라 초기화 실패!" << std::endl;
//        return -1;
//    }
//
//    return 0;
//} // 여기서 vc가 파괴되면서 카메라가 사라집니다.
//
//int main() {
//    test_cam();
//}

// main.cpp
#include <windows.h>
#include <mfapi.h>
#include <mfvirtualcamera.h>
#include <iostream>

#pragma comment(lib, "mfsensorgroup.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mfuuid.lib")

#include "wcam.h"
#include <iostream>
#include <wrl.h>
#include <fstream>
#include <vector>
using Microsoft::WRL::ComPtr;
void SaveSampleToBmp(IMFSample* pSample, UINT width, UINT height, const wchar_t* fileName)
{
    if (!pSample) return;

    ComPtr<IMFMediaBuffer> buffer;
    if (FAILED(pSample->GetBufferByIndex(0, &buffer))) return;

    BYTE* pData = nullptr;
    DWORD cbData = 0;
    if (FAILED(buffer->Lock(&pData, nullptr, &cbData))) return;

    // 1. NV12 크기 검증 (가로 * 세로 * 1.5)
    DWORD expectedNV12Size = width * height * 3 / 2;
    if (cbData < expectedNV12Size) {
        OutputDebugStringW(L"[오류] NV12 데이터 크기가 해상도와 일치하지 않습니다.\n");
        buffer->Unlock();
        return;
    }

    // 2. 변환 후 저장될 RGB32(32비트) 메모리 할당
    DWORD rgbSize = width * height * 4;
    std::vector<BYTE> rgbData(rgbSize, 0);

    // 3. NV12 -> RGB32 포맷 변환 루프
    BYTE* pY = pData;                           // Y 성분 시작 지점
    BYTE* pUV = pData + (width * height);       // UV 성분 시작 지점 (Y 데이터 바로 뒤)

    for (UINT y = 0; y < height; y++) {
        for (UINT x = 0; x < width; x++) {
            // YUV 인덱스 계산
            UINT yIdx = y * width + x;
            // UV는 2x2 픽셀이 1쌍을 공유하므로 x, y를 2로 나눔
            UINT uvIdx = ((y / 2) * (width / 2) + (x / 2)) * 2;

            int Y = pY[yIdx];
            int U = pUV[uvIdx];
            int V = pUV[uvIdx + 1];

            // YUV to RGB 변환 표준 공식
            int C = Y - 16;
            int D = U - 128;
            int E = V - 128;

            int R = (298 * C + 409 * E + 128) >> 8;
            int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B = (298 * C + 516 * D + 128) >> 8;

            // 0 ~ 255 값으로 고정 (Clamping)
            R = (R < 0) ? 0 : ((R > 255) ? 255 : R);
            G = (G < 0) ? 0 : ((G > 255) ? 255 : G);
            B = (B < 0) ? 0 : ((B > 255) ? 255 : B);

            // BMP는 BGRA(B, G, R, A순)로 메모리에 배치됩니다.
            UINT rgbIdx = (y * width + x) * 4;
            rgbData[rgbIdx + 0] = (BYTE)B; // Blue
            rgbData[rgbIdx + 1] = (BYTE)G; // Green
            rgbData[rgbIdx + 2] = (BYTE)R; // Red
            rgbData[rgbIdx + 3] = 0xFF;    // Alpha (불투명)
        }
    }

    // 4. 비트맵 헤더 설정 (최종 저장용 32비트 헤더)
    BITMAPFILEHEADER bfh = { 0 };
    bfh.bfType = 0x4D42; // "BM"
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + rgbSize;

    BITMAPINFOHEADER bih = { 0 };
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -(LONG)height; // 위아래 반전 방지 (Top-Down)
    bih.biPlanes = 1;
    bih.biBitCount = 32;          // 변환된 데이터가 32비트이므로 32 유지
    bih.biCompression = BI_RGB;
    bih.biSizeImage = rgbSize;

    // 5. 파일 쓰기
    std::ofstream file(fileName, std::ios::binary);
    if (file.is_open()) {
        file.write((char*)&bfh, sizeof(bfh));
        file.write((char*)&bih, sizeof(bih));
        file.write((char*)rgbData.data(), rgbSize); // 변환된 RGB 데이터 저장
        file.close();
        wprintf(L"NV12 -> BMP 변환 및 저장 완료: %s\n", fileName);
    }

    buffer->Unlock();
}

int test_cam() {
    // 1. COM 및 MF 초기화
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    hr = MFStartup(MF_VERSION);

    {
        WebcamCapture capture;
        // 웹캠 사양에 맞게 조절 (보통 640x480, 30fps는 대부분 지원)
        hr = capture.Initialize(640, 480, 30);

        if (FAILED(hr)) {
            std::cout << "초기화 실패: HRESULT 0x" << std::hex << hr << std::endl;
            return 0;
        }

        std::cout << "캡처 시작..." << std::endl;

        for (int i = 0; i < 10; ++i) { // 10프레임만 테스트
            ComPtr<IMFSample> sample;
            hr = capture.GetFrame(&sample);

            if (SUCCEEDED(hr) && sample) {
                DWORD bufferCount = 0;
                sample->GetBufferCount(&bufferCount);
                LONGLONG time = 0;
                sample->GetSampleTime(&time);

                std::cout << i << "번 프레임 수신 성공! (버퍼수: " << bufferCount
                    << ", 시간: " << time << ")" << std::endl;
                if (i == 9) {
                    SaveSampleToBmp(sample.Get(), 640, 480, L"capture_test.bmp");
                    std::cout << "saved to file" << std::endl;
                }

            }
            else {
                std::cout << i << "번 프레임 수신 실패 또는 대기 중..." << std::endl;
            }
            Sleep(33); // 약 30fps 간격
        }

    }

    MFShutdown();
    CoUninitialize();
    return 0;
}

int wmain()
{
    //test_cam();
    //return 0;
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        std::wcout << L"MFStartup failed: 0x"
            << std::hex << hr << std::endl;
        return -1;
    }

    IMFVirtualCamera* vcam = nullptr;

    // DLL에 등록한 MediaSource CLSID
    const wchar_t* clsid =
        L"{3CAD447D-F283-4AF4-A3B2-6F5363309F52}";

    hr = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser,
        L"My Virtual Camera",
        clsid,
        nullptr,
        0,
        &vcam
    );

    if (FAILED(hr))
    {
        std::wcout << L"MFCreateVirtualCamera failed: 0x"
            << std::hex << hr << std::endl;

        MFShutdown();
        return -1;
    }

    std::wcout << L"Virtual camera created" << std::endl;

    hr = vcam->Start(nullptr);

    if (FAILED(hr))
    {
        std::wcout << L"Start failed: 0x"
            << std::hex << hr << std::endl;

        vcam->Release();
        MFShutdown();
        return -1;
    }

    std::wcout << L"Camera started successfully" << std::endl;
    std::wcout << L"Open Camera / Zoom / OBS to check it" << std::endl;

    // 유지
    getchar();

    vcam->Stop();

    // Session lifetime이면 Remove 권장
    vcam->Remove();

    vcam->Release();

    MFShutdown();

    return 0;
}