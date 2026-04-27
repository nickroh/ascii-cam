# 🎥 ASCII-Cam

**Language:** [한국어](README.md) | [English](README.eng.md)

> **Windows Media Foundation 기반의 경량 가상 웹캠 아스키 아트 필터**

`ASCII-Cam Flow`는 노트북의 실제 웹캠 영상을 실시간으로 분석하여 아스키 아트(ASCII Art) 이미지로 변환하고, 이를 시스템 가상 카메라 장치로 송출하는 프로젝트입니다. **Media Foundation** 표준을 사용하여 최신 Windows 환경에서 높은 호환성과 성능을 유지합니다.

---

## 🛠 테크 스택 (Tech Stack)
* **언어:** C++ 20
* **이미지 처리:** OpenCV 4.x
* **시스템 API:** Windows Media Foundation (MF), Win32 API
* **IPC (프로세스 간 통신):** Named Shared Memory (Memory Mapped Files)
* **IDE:** Visual Studio 2022 / Windows SDK

---

## 🏗 시스템 아키텍처 (Architecture)

본 프로젝트는 성능 최적화와 안정성을 위해 **Provider**와 **Proxy**가 분리된 하이브리드 구조를 채택합니다.

1. **ASCII Provider (실행 파일):**
   - 실제 하드웨어 웹캠에서 데이터를 획득합니다.
   - OpenCV를 이용한 Gray-scale 변환 및 밝기 데이터 추출.
   - 픽셀 데이터를 아스키 문자 이미지 프레임으로 렌더링.
   - 공유 메모리에 비트맵 데이터를 기록.

2. **MF Device Proxy (DLL 드라이버):**
   - `IMFDeviceTransform` 인터페이스를 구현한 유저 모드 드라이버.
   - 공유 메모리에서 데이터를 읽어 시스템 미디어 스택으로 전송.
   - 디스코드, 줌 등에서 표준 웹캠으로 인식.

---

## 🚀 주요 기능 (Key Features)
* **Zero-Driver Installation:** 커널 드라이버 서명 없이 유저 모드에서 안전하게 작동.
* **High Performance:** 공유 메모리를 통한 초고속 데이터 전송으로 지연 시간(Latency) 최소화.
* **Live Control:** 전용 컨트롤러를 통해 아스키 모드 On/Off 및 필터 강도 실시간 조절.
* **Lightweight:** 최소한의 자원을 사용하여 시스템 부하 방지.

---

## 📅 프로젝트 로드맵 (Roadmap)
- [ ] Phase 1: OpenCV 기반 웹캠 캡처 및 아스키 변환 알고리즘 최적화
- [ ] Phase 2: Win32 공유 메모리(IPC) 인터페이스 구축
- [ ] Phase 3: Media Foundation 가상 카메라 DLL 구현 및 장치 등록
- [ ] Phase 4: 사용자 제어 GUI 개발 및 안정성 테스트
