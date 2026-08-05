==========================
용어정리
==========================


==========================
 Virtual Camera 구조 분석
==========================

[현재까지 분석된 파일]
✔ dllmain.cpp
✔ Activator.h
✔ Activator.cpp

--------------------------------------------------
1. 전체 구조
--------------------------------------------------

                 Windows
                     │
             COM Loader / Registry
                     │
                     ▼
          DllGetClassObject()
                     │
                     ▼
              ClassFactory
                     │
          CreateInstance()
                     │
                     ▼
                Activator
                     │
        ActivateObject()
                     │
                     ▼
               MediaSource
                     │
                     ▼
               MediaStream
                     │
                     ▼
            FrameGenerator
                     │
                     ▼
              Video Frames

※ MediaSource 이후는 아직 코드 미분석

--------------------------------------------------
2. DLL의 역할
--------------------------------------------------

이 프로젝트는 실행 파일(EXE)이 아니라
COM DLL 기반 Media Foundation Virtual Camera이다.

Windows가 DLL을 로드하고,
COM 객체를 생성하여
Media Foundation에 제공하는 구조이다.

--------------------------------------------------
3. dllmain.cpp
--------------------------------------------------

역할

- DLL 시작점
- COM 등록/해제
- ClassFactory 제공
- Activator 생성 시작

주요 함수

DllMain()
    DLL 로드/언로드 처리
    Trace 초기화
    Error Logging 초기화

DllGetClassObject()
    CLSID 요청 시
    ClassFactory 반환

DllCanUnloadNow()
    COM 객체가 남아있는지 확인

DllRegisterServer()
    Registry 등록

DllUnregisterServer()
    Registry 삭제

--------------------------------------------------
4. ClassFactory
--------------------------------------------------

Windows는 직접 MediaSource를 생성하지 않는다.

Windows
    │
    ▼
ClassFactory
    │
CreateInstance()
    │
    ▼
Activator 생성

즉 COM 객체 생성기 역할

--------------------------------------------------
5. Activator
--------------------------------------------------

상속

Activator
    ├── IMFActivate
    └── IMFAttributes

역할

Media Foundation이
실제 MediaSource를 생성하도록 연결하는 객체

직접 영상을 생성하지 않는다.

--------------------------------------------------
6. Activator::Initialize()
--------------------------------------------------

_source = winrt::make_self<MediaSource>();

↓

MediaSource 객체 생성

↓

Attribute 설정

↓

MediaSource Initialize()

아직 카메라는 시작되지 않는다.

--------------------------------------------------
7. ActivateObject()
--------------------------------------------------

가장 중요한 함수

Media Foundation이

"MediaSource를 주세요"

라고 요청하면

Activator가

MediaSource를 반환한다.

즉

Frame Server
      │
      ▼
ActivateObject()
      │
      ▼
MediaSource 반환

카메라 시작 X

객체 제공 O

--------------------------------------------------
8. 카메라는 언제 시작되는가?
--------------------------------------------------

현재까지 추정되는 실행 순서

Windows

↓

Load DLL

↓

DllMain()

↓

DllGetClassObject()

↓

ClassFactory

↓

CreateInstance()

↓

Activator

↓

ActivateObject()

↓

MediaSource 반환

↓

MediaSource::Start()

↓

MediaStream

↓

RequestSample()

↓

FrameGenerator

↓

영상 생성

즉

ActivateObject()는
카메라를 켜는 함수가 아니다.

MediaSource::Start()가
실제 스트리밍 시작 지점일 가능성이 매우 높다.

--------------------------------------------------
9. _source의 의미
--------------------------------------------------

private:

winrt::com_ptr<MediaSource> _source;

Activator는

MediaSource 하나만 관리한다.

--------------------------------------------------
10. MediaSource 초기화
--------------------------------------------------

Activator::Initialize()
    _source = winrt::make_self<MediaSource>();
    MF_VIRTUALCAMERA_PROVIDE_ASSOCIATED_CAMERA_SOURCES = 1
    MFT_TRANSFORM_CLSID_Attribute = CLSID_VCam
    _source->Initialize(this)

MediaSource::Initialize()
    - attributes가 있으면 복사해서 자신의 IMFAttributes로 유지
    - 센서 프로파일 컬렉션 생성:
        * KSCAMERAPROFILE_Legacy: FRT<=30
        * KSCAMERAPROFILE_HighFrameRate: FRT>=60
    - MF_DEVICEMFT_SENSORPROFILE_COLLECTION 속성에 컬렉션 저장
    - AppX가 존재하면 MF_VIRTUALCAMERA_CONFIGURATION_APP_PACKAGE_FAMILY_NAME 설정
    - 각 스트림에서 스트림 디스크립터 생성 후 MFCreatePresentationDescriptor
    - MFCreateEventQueue로 이벤트 큐 생성

MediaSource 역할
    - IMFMediaSourceEx / IMFGetService / IKsControl / IMFSampleAllocatorControl 구현
    - CreatePresentationDescriptor, GetCharacteristics, Start/Stop, Shutdown 제공
    - stream 수는 _numStreams = 1
    - GetCharacteristics는 MFMEDIASOURCE_IS_LIVE

--------------------------------------------------
11. MediaSource 스트림 관리
--------------------------------------------------

MediaSource 생성자
    - MediaStream 객체 생성
    - stream->Initialize(this, index)
    - _streams에 attach

MediaSource::Start()
    - 전달된 프리젠테이션 디스크립터에서 스트림 개수 확인
    - 선택된 스트림의 id를 확인하고 _descriptor에서 같은 스트림의 선택 상태 비교
    - 선택되면 _descriptor->SelectStream(index) 후
      _streams[index]->Start(type) 호출
    - 선택 해제되면 _streams[index]->Stop()
    - MESourceStarted 이벤트 큐에 추가

MediaSource::Stop()
    - 모든 스트림 Stop
    - _descriptor에서 DeselectStream
    - MESSourceStopped 이벤트 큐에 추가

Allocator 관리
    - SetDefaultAllocator는 지정된 스트림에 대해 MediaStream::SetAllocator 호출
    - GetAllocatorUsage는 _streams[index]->GetAllocatorUsage 반환

--------------------------------------------------
12. MediaStream 구조
--------------------------------------------------

MediaStream::Initialize()
    - stream category = PINNAME_VIDEO_CAPTURE
    - MF_DEVICESTREAM_STREAM_ID = index
    - MF_DEVICESTREAM_FRAMESERVER_SHARED = 1
    - MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES = Color
    - event queue 생성
    - media type 리스트 생성:
        * RGB32 1280x720 30fps
        * NV12 1280x720 30fps
    - MFCreateStreamDescriptor로 스트림 디스크립터 생성
    - 현재 미디어 타입을 types[1] (NV12)로 설정

MediaStream::Start()
    - allocator 초기화(10 샘플, 선택된 type)
    - 상태를 RUNNING으로 변경
    - 캡처 쓰레드 생성: CaptureLoop()
    - _converter.Initialize(1280,720)
    - MEStreamStarted 이벤트 큐에 추가

MediaStream::Stop()
    - 캡처 스레드 join
    - _capture.Shutdown()
    - allocator 해제
    - MEStreamStopped 이벤트 큐에 추가
    - 상태 STOPPED

--------------------------------------------------
13. Capture & 샘플 전송 흐름
--------------------------------------------------

CaptureLoop()
    - WebcamCapture.Initialize(1280,720,30)
    - ColorConverter.Initialize(1280,720)
    - while (_state == RUNNING):
        * _capture.GetFrame(&webcamSample)
        * _converter.Convert(webcamSample, &nv12Sample)
        * _latestFrame = nv12Sample

WebcamCapture
    - MFEnumDeviceSources로 비디오 캡처 장치 나열
    - friendly name에 "Virtual"이 들어가면 건너뜀
    - 첫 번째 실제 카메라를 활성화
    - MFCreateSourceReaderFromMediaSource 생성
    - native type 열거 후 YUY2 1280x720 30fps 우선 선택
    - YUY2가 없으면 다른 YUY2 형식으로 폴백

ColorConverter
    - CLSID_CColorConvertDMO 인스턴스 생성
    - 입력 YUY2 1280x720 30fps
    - 출력 NV12 1280x720 30fps
    - ProcessInput/ProcessOutput로 변환

MediaStream::RequestSample()
    - _latestFrame를 가져옴
    - ContiguousBuffer로 변환 후 메모리 버퍼 생성
    - 버퍼를 복사하여 새 IMFSample에 추가
    - 샘플 시간과 duration 설정
    - MEMediaSample 이벤트로 큐에 전송

결론
    - 실제 카메라 프레임은 CaptureLoop에서 지속 갱신됨
    - RequestSample은 최신 프레임을 복사하여 가상 카메라 클라이언트에 전달
    - 따라서 MediaStream은 실시간 카메라 데이터를 가상 카메라 출력으로 중계

--------------------------------------------------
14. FrameGenerator 위치
--------------------------------------------------

FrameGenerator는 GPU/CPU 렌더링 기반 샘플 생성을 지원하는 유틸리티임
    - D3D manager가 있으면 GPU 텍스처 생성
    - 없으면 WIC 비트맵 + D2D render target 생성
    - 텍스트와 HSL 블록을 그려서 샘플을 만듦

현재 코드 흐름에서는 MediaStream::RequestSample()가 FrameGenerator를 직접 호출하지 않음
    - MediaStream::Start()에 _generator.Initialize 또는 _generator.EnsureRenderTarget 관련 주석이 존재
    - 실제로는 WebcamCapture + ColorConverter 경로가 사용됨

따라서 FrameGenerator는 향후 가상 프레임 생성 / 렌더링 경로를 위한 준비 코드로 보임

--------------------------------------------------
15. 요약
--------------------------------------------------

- 이 프로젝트는 COM DLL 기반 Media Foundation 가상 카메라임
- `Activator`는 MediaSource 객체를 생성하고 Media Foundation에 제공함
- `ActivateObject()`는 `MediaSource` 인터페이스를 반환할 뿐 카메라를 바로 시작하지 않음
- 실제 스트리밍은 `MediaSource::Start()` / `MediaStream::Start()` 시점에 시작됨
- `MediaStream`은 물리 카메라를 직접 캡처하고 NV12 프레임을 생성하여 가상 카메라 소비자에 전달함
- `_source`는 Activator가 관리하는 단일 MediaSource 인스턴스이며, 그 아래에 1개의 MediaStream이 있다

--------------------------------------------------
16. 추가 관찰
--------------------------------------------------

- DllRegisterServer는 반드시 HKLM에 CLSID 등록을 수행함
- `MF_FRAMESERVER_CLIENTCONTEXT_CLIENTPID`를 사용해 호출자 PID를 추적함
- `MF_DEVICESTREAM_FRAMESERVER_SHARED` 속성으로 Frame Server 공유 모드를 표시함
- `IMFGetService`와 IKsControl 구현은 현재 대부분 Unsupported/ERROR_SET_NOT_FOUND로 처리됨
- `MediaSource::SetMediaType()`는 현재 단순 로깅만 수행함

FrameGenerator를 직접 호출하지 않는다.

--------------------------------------------------
17. Lag 원인 및 개선 포인트
--------------------------------------------------

현재 활성화 후 카메라가 "약간 느리게" 동작하는 원인은 주로 다음 경로에서 발생할 가능성이 큽니다.

1) YUY2 -> NV12 변환
    - `WebcamCapture`는 카메라에서 YUY2로 프레임을 받음
    - `ColorConverter::Convert()`가 DMO 기반 변환을 수행함
    - MediaStream::RequestSample()는 매 프레임마다 새 NV12 샘플을 생성하고 버퍼를 복사함

2) 샘플 복사 비용
    - `RequestSample()` 내부에서 `ConvertToContiguousBuffer()` 호출 후
      새 메모리 버퍼 생성, Lock, CopyMemory, Unlock을 수행함
    - 이 반복 작업은 CPU 부담이 크고 지연을 유발할 수 있음

3) 스레드 동기화 / 최신 프레임 대기
    - CaptureLoop()는 `_latestFrame`을 계속 갱신하고
      RequestSample()는 잠금으로 최신 프레임을 읽어옴
    - 적절한 버퍼 재사용 없이 매번 새 샘플을 생성하면 지연이 커짐

개선 제안
    - 카메라 출력에서 가능한 한 NV12를 직접 받도록 구성
      * YUY2 대신 NV12를 SourceReader에서 직접 선택하면 변환 비용 제거
    - `RequestSample()`에서 새 샘플/버퍼를 매번 생성하지 않도록 개선
      * allocator가 제공되면 그 샘플의 버퍼에 바로 쓰기
      * 가능한 경우 ContiguousBuffer 대신 기존 샘플을 재사용
    - GPU/하드웨어 가속 변환 사용
      * 현재 `ColorConverter`는 CPU 기반 DMO 변환임
      * D3D 매니저가 제공되는 경우 GPU 변환 또는 VideoProcessorMFT 사용을 고려
    - 해상도/프레임 레이트 낮추기
      * 1280x720 30fps는 CPU와 메모리에 부담
      * 720p/15fps 또는 640x480으로 임시 설정하면 반응성이 개선됨
    - FrameGenerator 대신 직접 카메라 샘플을 전달
      * 현재 FrameGenerator는 실제 경로에 연결되지 않음
      * 인위적인 렌더링 경로를 추가하면 추가 오버헤드가 생길 수 있음

추가 디버그 포인트
    - `MediaStream::SetStreamState()`에 `if (_state = value)`라는 할당 버그가 있음
      * 상태 로직이 의도대로 동작하지 않을 수 있으므로 수정 필요
    - 캡처 스레드가 프레임을 너무 빨리 읽거나 너무 늦게 전달하는지 확인
      * `GetFrame()`이 블록킹인지, `ReadSample()` 대기 시간이 어느 정도인지 측정

정리
    - 가장 큰 병목은 변환 + 복사 + 샘플 생성 반복입니다.
    - 가능한 한 변환 단계를 줄이고, 제공된 allocator/샘플 재사용을 늘리면 활성화 지연을 줄일 수 있습니다.

--------------------------------------------------
10. winrt::make_self
--------------------------------------------------

_source = winrt::make_self<MediaSource>();

의 의미

↓

COM 객체(MediaSource) 생성

↓

참조 카운트 초기화

↓

com_ptr에 저장

개념적으로는

MediaSource* p = new MediaSource();

+

COM Reference Counting

+

Smart Pointer

를 합친 것과 비슷하다.

--------------------------------------------------
11. 현재까지 확인된 객체 관계
--------------------------------------------------

Windows

↓

ClassFactory

↓

Activator
    │
    └────────────┐
                 │
                 ▼
            MediaSource
                 │
                 ▼
            MediaStream
                 │
                 ▼
          FrameGenerator

--------------------------------------------------
12. 현재까지 확인된 책임
--------------------------------------------------

DllMain
    DLL 초기화

ClassFactory
    COM 객체 생성

Activator
    MediaSource 생성 및 제공

MediaSource
    (미분석)
    실제 Media Foundation Source

MediaStream
    (미분석)
    Video Stream 제공

FrameGenerator
    (미분석)
    실제 영상 생성

--------------------------------------------------
13. 앞으로 확인해야 할 핵심
--------------------------------------------------

□ MediaSource::Initialize()

□ MediaSource::Start()

□ MediaStream 생성 위치

□ RequestSample()

□ FrameGenerator 연결

□ 실제 프레임 생성 루프

□ EventQueue 구조

□ Media Foundation 이벤트 흐름

=====
00004A34:Camera Found: HD Webcam
00004A34:[Wcam] NativeType 0 -> {47504A4D-0000-0010-8000-00AA00389B71} 1280x720 30/1
00004A34:[Wcam] NativeType 1 -> {47504A4D-0000-0010-8000-00AA00389B71} 320x180 30/1
00004A34:[Wcam] NativeType 2 -> {47504A4D-0000-0010-8000-00AA00389B71} 320x240 30/1
00004A34:[Wcam] NativeType 3 -> {47504A4D-0000-0010-8000-00AA00389B71} 352x288 30/1
00004A34:[Wcam] NativeType 4 -> {47504A4D-0000-0010-8000-00AA00389B71} 424x240 30/1
00004A34:[Wcam] NativeType 5 -> {47504A4D-0000-0010-8000-00AA00389B71} 640x360 30/1
00004A34:[Wcam] NativeType 6 -> {47504A4D-0000-0010-8000-00AA00389B71} 640x480 30/1
00004A34:[Wcam] NativeType 7 -> {47504A4D-0000-0010-8000-00AA00389B71} 848x480 30/1
00004A34:[Wcam] NativeType 8 -> {47504A4D-0000-0010-8000-00AA00389B71} 960x540 30/1
00004A34:[Wcam] NativeType 9 -> {32595559-0000-0010-8000-00AA00389B71} 1280x720 10/1
00004A34:[Wcam] NativeType 10 -> {32595559-0000-0010-8000-00AA00389B71} 320x180 30/1
00004A34:[Wcam] NativeType 11 -> {32595559-0000-0010-8000-00AA00389B71} 320x240 30/1
00004A34:[Wcam] NativeType 12 -> {32595559-0000-0010-8000-00AA00389B71} 352x288 30/1
00004A34:[Wcam] NativeType 13 -> {32595559-0000-0010-8000-00AA00389B71} 424x240 30/1
00004A34:[Wcam] NativeType 14 -> {32595559-0000-0010-8000-00AA00389B71} 640x360 30/1
00004A34:[Wcam] NativeType 15 -> {32595559-0000-0010-8000-00AA00389B71} 640x480 30/1
00004A34:[Wcam] NativeType 16 -> {32595559-0000-0010-8000-00AA00389B71} 848x480 20/1
00004A34:[Wcam] NativeType 17 -> {32595559-0000-0010-8000-00AA00389B71} 960x540 15/1
00004A34:[Wcam] Selected YUY2 1280x720 10/1
