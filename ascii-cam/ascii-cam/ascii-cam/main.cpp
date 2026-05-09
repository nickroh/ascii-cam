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

int wmain()
{
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