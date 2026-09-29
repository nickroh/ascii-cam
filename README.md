# ASCII-Cam

**언어:** [한국어](README.md) | [English](README.eng.md)

Windows Media Foundation으로 실제 웹캠 영상을 ASCII 필터로 처리해 소프트웨어 가상 카메라로 제공하는 C++ 프로젝트입니다.

## 현재 구현

- `ascii-cam-source`: Media Foundation 소프트웨어 카메라 DLL입니다. `IMFMediaSource`/`IMFMediaStream`을 제공하고, Source Reader로 실제 웹캠을 캡처합니다.
- `ascii-cam`: 웹캠 캡처와 Media Foundation 샘플 처리를 확인하는 콘솔 테스트 프로젝트입니다.
- 필터는 `FilterOption::ASCII`로 초기화되며, CPU 경로와 Direct3D 11/DXGI 기반 경로를 지원합니다.
- 출력 미디어 형식은 RGB32와 NV12이고 기본 스트림 형식은 1280x720, 30 FPS입니다.

현재 저장소에는 GUI, OpenCV 기반 실행 경로, Named Shared Memory 기반 프레임 전달, HLSL 셰이더 구현이 포함되어 있지 않습니다. `SharedMemory`와 일부 OpenCV 코드는 이전 실험용 코드로 남아 있지만 활성 경로가 아닙니다.

## 요구 사항

- Windows 10 이상
- Visual Studio 2022
- Windows 10/11 SDK 및 C++ 데스크톱 개발 도구
- Media Foundation, Direct3D 11, Direct2D, DirectWrite 지원 환경

## 빌드

Visual Studio에서 각 솔루션을 열고 `Release | x64` 구성을 선택해 빌드합니다.

1. `ascii-cam-source/ascii-cam.sln`을 빌드해 가상 카메라 DLL을 만듭니다.
2. `ascii-cam/ascii-cam/ascii-cam.sln`을 빌드해 캡처 테스트 실행 파일을 만듭니다.

두 솔루션은 서로 다른 프로젝트이며, 현재 Linux 환경에서는 Windows 전용 코드를 빌드할 수 없습니다.

## DLL 등록

관리자 권한 명령 프롬프트에서 빌드된 DLL에 `regsvr32`를 실행합니다.

```bat
regsvr32 path\to\ascii-cam.dll
```

등록 해제는 다음과 같습니다.

```bat
regsvr32 /u path\to\ascii-cam.dll
```

DLL은 `HKLM`에 등록되므로 등록과 해제에 관리자 권한이 필요합니다. 등록 후 Discord, Zoom 등 Media Foundation 카메라를 사용하는 애플리케이션에서 장치를 선택할 수 있습니다.

## 상태

핵심 캡처, ASCII 필터, 소프트웨어 카메라 등록 경로가 구현되어 있습니다. 사용자 인터페이스, 설치 패키지, 자동화된 Windows 테스트, 성능 목표 검증은 아직 남아 있습니다.