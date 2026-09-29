# ASCII-Cam

**언어:** [한국어](README.md) | [English](README.eng.md)

# ASCII-Cam은 Windows Media Foundation 기반 C++ 소프트웨어 카메라 프로젝트입니다. 물리 웹캠의 프레임을 읽고 Direct3D 11 컴퓨트 셰이더로 ASCII 스타일의 명암 패턴을 만든 뒤, Media Foundation 가상 카메라 소스로 노출하는 것을 목표로 합니다.

이 문서는 저장소의 현재 코드 경로를 설명합니다. 코드에 남아 있는 실험용 구성요소와 실제 동작 중인 경로를 구분하고, 새 팀원이 빌드와 실행 흐름을 추적할 수 있도록 합니다.

## 저장소 구성

| 경로 | 역할 |
| --- | --- |
| `ascii-cam-source/` | Media Foundation 소프트웨어 카메라 소스 DLL. COM 활성화, `IMFMediaSource`/`IMFMediaStream`, 웹캠 캡처와 ASCII 후처리를 구현합니다. |
| `ascii-cam/` | DLL의 CLSID를 사용해 현재 세션용 가상 카메라를 만들고 시작하는 콘솔 호스트/테스트 프로젝트입니다. 일부 캡처 확인 코드는 `main.cpp`에 남아 있습니다. |
| `README.md`, `README.eng.md` | 한국어 및 영어 기술 개요와 개발 가이드입니다. |

주요 코드 파일은 다음과 같습니다.

| 파일 | 책임 |
| --- | --- |
| `ascii-cam-source/ascii-cam/dllmain.cpp` | DLL 진입점, COM 클래스 팩토리, CLSID 등록/해제. |
| `ascii-cam-source/ascii-cam/Activator.cpp` | Media Foundation이 요청한 가상 카메라 객체를 활성화합니다. |
| `ascii-cam-source/ascii-cam/MediaSource.cpp` | 카메라 소스 특성, 프레젠테이션 디스크립터, 스트림 선택 및 시작/정지 이벤트를 관리합니다. |
| `ascii-cam-source/ascii-cam/MediaStream.cpp` | 출력 미디어 타입, 캡처 스레드, 필터, 최신 프레임 저장 및 `MEMediaSample` 전달을 담당합니다. |
| `ascii-cam-source/ascii-cam/WebCamCapture.cpp` | 물리 카메라 열거, 네이티브 타입 선택, Source Reader를 통한 프레임 캡처를 구현합니다. |
| `ascii-cam-source/ascii-cam/Filter.cpp` | 샘플 버퍼를 잠그고 선택된 후처리 엔진을 호출합니다. |
| `ascii-cam-source/ascii-cam/AsciiEngine.cpp` | D3D11 리소스와 컴퓨트 셰이더를 만들고 NV12 Y 평면에 ASCII 명암 패턴을 기록합니다. |
| `ascii-cam-source/ascii-cam/FrameGenerator.cpp` | Direct2D/WIC 프레임 생성 및 RGB32/NV12 변환을 구현한 별도 경로입니다. 현재 라이브 `RequestSample` 흐름에서는 호출되지 않습니다. |

## 아키텍처와 프레임 흐름

```mermaid
flowchart LR
	Client[Media Foundation 카메라 클라이언트] -->|MFCreateVirtualCamera / Start| Host[ascii-cam 호스트]
	Host -->|CLSID 활성화| Activator[Activator]
	Activator --> Source[MediaSource]
	Source --> Stream[MediaStream]
	Stream -->|Start| Capture[WebcamCapture / Source Reader]
	Capture -->|최신 샘플| Filter[Filter]
	Filter --> Ascii[AsciiEngine / D3D11 compute]
	Ascii -->|수정된 Y 평면| Latest[최신 프레임 슬롯]
	Client -->|RequestSample| Stream
	Latest -->|샘플 복사 및 MEMediaSample| Client
```

1. 호스트는 `MFStartup` 후 `MFCreateVirtualCamera`를 호출합니다. 현재 구현은 이름을 `Ascii Cam`, lifetime을 `MFVirtualCameraLifetime_Session`, access를 `MFVirtualCameraAccess_CurrentUser`로 지정하고 DLL의 CLSID `{3CAD447D-F283-4AF4-A3B2-6F5363309F52}`를 전달합니다.
2. Media Foundation이 DLL의 클래스 팩토리를 호출하면 `Activator`가 `MediaSource`를 만듭니다. 소스는 현재 하나의 비디오 스트림을 제공합니다.
3. 클라이언트가 스트림을 시작하면 `MediaStream::Start`가 선택된 크기와 프레임 속도로 `WebcamCapture`를 초기화하고 ASCII 필터를 준비한 뒤 캡처 스레드를 시작합니다.
4. 캡처 스레드는 Source Reader에서 프레임을 읽고 `Filter::Process`에 전달합니다. 성공한 최신 샘플 하나를 잠금으로 보호되는 `_latestFrame`에 저장합니다. 오래된 프레임을 큐에 쌓지 않는 최신 프레임 방식입니다.
5. 프레임 서버가 `RequestSample`을 호출하면 현재 `_latestFrame`을 복사하고 시간/길이/토큰 메타데이터를 설정한 `IMFSample`을 `MEMediaSample` 이벤트로 보냅니다.
6. `Stop`과 `Shutdown`은 스트림과 이벤트 큐, 캡처 리소스를 정리하도록 구성되어 있습니다. 다만 현재 스레드 중지 동작의 문제는 [알려진 제약](#현재-제약과-개선-항목)에 기록했습니다.

## 캡처와 미디어 타입

- `MediaStream`은 1280x720, 30/1 FPS를 기본값으로 사용합니다. 스트림 디스크립터에는 RGB32와 NV12가 등록되며, 현재 코드는 초기 기본 타입으로 NV12를 선택합니다.
- `WebcamCapture`는 시스템 카메라를 열거해 이름에 `Virtual`이 포함된 장치를 건너뛰고 첫 번째로 열 수 있는 장치를 사용합니다. 특정 장치를 선택하는 설정 UI나 장치 ID 옵션은 없습니다.
- 캡처기는 요청한 해상도와 일치하는 네이티브 모드 중 프레임 속도가 높은 모드를 고릅니다. 일치하는 해상도가 없으면 YUY2 네이티브 모드 중 하나를 대체 모드로 선택합니다.
- 선택한 네이티브 타입이 YUY2가 아니면 Source Reader 출력 타입을 NV12로 설정해 변환을 요청합니다. YUY2를 선택하면 그 타입을 그대로 유지합니다.
- Source Reader의 D3D11/DXGI 연결 생성이 실패하면 캡처 리더를 CPU 속성으로 다시 만들도록 되어 있습니다. 이 fallback은 캡처 디코딩 경로에 대한 것이며 ASCII 엔진 자체의 소프트웨어 대체 구현은 아닙니다.

## ASCII 필터 구현

현재 활성 필터는 `MediaStream::Start`에서 `FilterOption::ASCII`로 초기화됩니다.

1. `Filter`는 첫 번째 Media Foundation 버퍼를 `IMF2DBuffer::Lock2D`로 잠그고 행 pitch를 전달합니다. 2D 버퍼 인터페이스가 없으면 선형 버퍼로 잠급니다.
2. `AsciiEngine`은 하드웨어 D3D11 디바이스, R8 입력 텍스처, RGBA 출력 텍스처, staging 텍스처 및 셰이더를 만듭니다. 셰이더는 코드에 내장된 HLSL을 시작 시 컴파일합니다.
3. 프레임마다 Y 평면을 GPU 입력 텍스처로 복사하고 컴퓨트 셰이더를 실행합니다. 현재 셰이더는 8x16 픽셀 셀의 평균 휘도를 구하고 비선형 톤 보정 후 10개 내장 글리프 비트맵 중 하나를 선택합니다.
4. 결과 텍스처는 CPU staging 메모리로 복사됩니다. 각 출력 픽셀의 첫 번째 채널(검정 또는 흰색)을 원본 버퍼의 Y 평면에 덮어씁니다. NV12의 UV 평면은 그대로 두므로 색차 정보는 입력 프레임에서 유지됩니다.

이 경로는 CPU에서 처리하는 ASCII 엔진이 아닙니다. 카메라 읽기 쪽에 CPU fallback이 있더라도 `AsciiEngine::Initialize`는 하드웨어 D3D11 디바이스 생성을 요구합니다.

## 빌드와 실행

### 요구 사항

- Windows 10 이상 및 Media Foundation 지원
- Visual Studio 2022, C++ 데스크톱 개발 워크로드, Windows 10/11 SDK
- v143 플랫폼 도구 집합 및 C++20 컴파일러
- D3D11 하드웨어 지원
- 소스 프로젝트에서 참조하는 Windows Implementation Library(WIL) NuGet 패키지 복원

### 빌드

Visual Studio에서 각 솔루션을 열고 `Release | x64`를 선택합니다.

1. `ascii-cam-source/ascii-cam.sln`을 빌드합니다. 결과물은 Media Foundation 카메라 소스 DLL입니다.
2. `ascii-cam/ascii-cam/ascii-cam.sln`을 빌드합니다. 결과물은 세션 가상 카메라를 만드는 콘솔 호스트입니다.

두 프로젝트는 분리되어 있습니다. 호스트는 소스 DLL의 CLSID를 사용하므로, 가상 카메라를 만들기 전에 해당 DLL이 Windows에 등록되어 있어야 합니다. 저장소는 Windows 전용이며 Linux에서는 이 솔루션을 빌드하거나 가상 카메라를 실행할 수 없습니다.

### DLL 등록 및 카메라 실행

64비트 관리자 권한 명령 프롬프트에서 DLL을 등록합니다. `path\\to\\ascii-cam.dll`은 실제 빌드 출력 경로로 바꿉니다.

```bat
%SystemRoot%\\System32\\regsvr32.exe path\\to\\ascii-cam.dll
```

`DllRegisterServer`는 DLL의 CLSID를 `HKLM\\Software\\Classes\\CLSID\\{...}\\InprocServer32`에 등록합니다. 등록 해제:

```bat
%SystemRoot%\\System32\\regsvr32.exe /u path\\to\\ascii-cam.dll
```

그 다음 콘솔 호스트를 실행합니다. 성공하면 `Ascii Cam` 세션 카메라를 만들고 시작하며, 키 입력을 기다립니다. 종료 시 `Stop`과 `Remove`를 호출합니다. 다른 카메라 애플리케이션에서 확인하려면 호스트가 실행 중인 동안 장치 목록을 새로 고칩니다. 앱에서 DLL을 찾지 못하면 DLL 등록과 CLSID가 일치하는지 확인합니다.

### 진단

- 호스트의 `MFStartup`, `MFCreateVirtualCamera`, `Start` 실패는 콘솔에 HRESULT로 출력됩니다.
- DLL은 `WINTRACE`/WIL 결과 로깅을 사용합니다. Visual Studio의 디버그 출력에서 장치 열거와 Media Foundation 오류를 확인할 수 있습니다.
- 웹캠이 열리지 않으면 다른 앱이 장치를 독점 중인지, Windows 카메라 권한이 허용되어 있는지 확인합니다.
- 로그에 D3D11 생성 또는 DXGI manager 생성 실패가 있으면 캡처는 CPU 설정으로 재시도할 수 있지만, ASCII 필터는 D3D11 하드웨어가 필요합니다.

## 현재 제약과 개선 항목

아래 내용은 문서상 기대 동작이 아니라 코드에서 확인한 현재 상태입니다. 기능 확장 전 먼저 해결하거나 테스트해야 할 계약입니다.

- **출력 타입과 실제 샘플 형식 불일치:** 스트림은 RGB32/NV12를 광고하지만 `RequestSample`은 요청된 타입을 확인하거나 변환하지 않습니다. `_latestFrame`의 첫 버퍼를 복사해 반환하므로 실제 캡처 형식에 따라 클라이언트가 광고 타입과 다른 버퍼를 받을 수 있습니다. `FrameGenerator`의 변환 경로는 현재 이 요청 경로에 연결되어 있지 않습니다.
- **YUY2 fallback과 필터 입력 계약:** YUY2를 그대로 읽는 경우에도 필터는 샘플 시작 주소를 NV12 Y 평면처럼 처리합니다. YUY2의 패킹 레이아웃을 풀어 처리하는 변환 경로가 없어 필터 결과가 올바르지 않을 수 있습니다.
- **캡처 타임스탬프:** `RequestSample`은 원본 캡처 timestamp를 전달하지 않고 `MFGetSystemTime()` 및 고정 duration `333333`(100ns 단위)을 지정합니다. 실제 카메라 속도와 동기화되지 않을 수 있습니다.
- **첫 프레임 요청:** 아직 캡처된 프레임이 없으면 `RequestSample`은 이벤트를 보내지 않고 `S_OK`를 반환합니다. 대기, 재시도, 실패 이벤트 정책은 별도로 정의되어 있지 않습니다.
- **중지 스레드 신호:** `Stop`은 캡처 스레드를 `join`하기 전에 stop token을 요청하지 않고, 루프가 확인하는 상태도 join 이후에 변경합니다. 정상 캡처 중 Stop이 반환되지 않을 수 있으므로 스레드 종료 순서를 수정하고 회귀 테스트해야 합니다.
- **오류 전파:** `Filter`는 `AsciiEngine::Process`의 HRESULT를 확인하지 않고 버퍼 잠금도 일부 오류 경로에서 정리하지 않습니다. D3D 실패가 스트림 오류로 전달된다고 가정하면 안 됩니다.
- **범위:** GUI, 설치 프로그램, 장치 선택 UI, 자동화된 Windows 통합 테스트, 성능 목표 검증은 없습니다. `ColorConverter`는 deprecated 표시가 있고 `FrameGenerator`는 라이브 요청 경로에서 사용되지 않습니다. `SharedMemory`와 이전 OpenCV 기반 실험 코드도 현재 카메라 경로가 아닙니다.

## 다음 개발 작업

1. 캡처 입력을 한 가지 명시적 내부 포맷(NV12 등)으로 정규화하고 RGB32/NV12 출력 타입 각각의 변환을 구현합니다.
2. 원본 timestamp/duration을 보존하고 첫 프레임 대기 및 장치 분리 이벤트를 정의합니다.
3. stop token 요청과 상태 전이를 정리해 시작/중지/재시작 및 장치 분리 테스트를 추가합니다.
4. 필터의 버퍼 잠금과 HRESULT 전파를 안전하게 만들고 D3D11 사용 불가 환경의 정책을 결정합니다.
5. Windows CI 또는 개발자용 체크리스트에서 포맷 협상, 카메라 클라이언트 연결, 등록/해제를 검증합니다.