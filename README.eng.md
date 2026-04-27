# 🎥 ASCII-Cam

**Language:** [한국어](README.md) | [English](README.eng.md)

> **Lightweight Virtual Webcam ASCII Art Filter Based on Windows Media Foundation**

`ASCII-Cam Flow` is a project that analyzes real webcam footage from your notebook in real-time, converts it to ASCII Art images, and outputs it through a system virtual camera device. Using the **Media Foundation** standard, it maintains high compatibility and performance in modern Windows environments.

---

## 🛠 Tech Stack
* **Language:** C++ 20
* **Image Processing:** OpenCV 4.x
* **System API:** Windows Media Foundation (MF), Win32 API
* **IPC (Inter-Process Communication):** Named Shared Memory (Memory Mapped Files)
* **IDE:** Visual Studio 2022 / Windows SDK

---

## 🏗 System Architecture

This project adopts a hybrid architecture with separated **Provider** and **Proxy** for performance optimization and stability.

1. **ASCII Provider (Executable):**
   - Acquires data from actual hardware webcam.
   - Grayscale conversion and brightness data extraction using OpenCV.
   - Renders pixel data into ASCII character image frames.
   - Writes bitmap data to shared memory.

2. **MF Device Proxy (DLL Driver):**
   - Implements `IMFDeviceTransform` interface as a user-mode driver.
   - Reads data from shared memory and transmits it to the system media stack.
   - Recognized as a standard webcam in Discord, Zoom, etc.

---

## 🚀 Key Features
* **Zero-Driver Installation:** Operates safely in user-mode without kernel driver signing.
* **High Performance:** Minimizes latency through ultra-fast data transmission via shared memory.
* **Live Control:** Toggle ASCII mode On/Off and adjust filter intensity in real-time through a dedicated controller.
* **Lightweight:** Prevents system load by using minimal resources.

---

## 📅 Project Roadmap
- [ ] Phase 1: Optimize OpenCV-based webcam capture and ASCII conversion algorithm
- [ ] Phase 2: Build Win32 shared memory (IPC) interface
- [ ] Phase 3: Implement Media Foundation virtual camera DLL and device registration
- [ ] Phase 4: Develop user control GUI and stability testing
