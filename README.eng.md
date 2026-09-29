# ASCII-Cam

**Language:** [한국어](README.md) | [English](README.eng.md)

A C++ project that captures a physical webcam through Windows Media Foundation, applies an ASCII filter, and exposes the result as a software virtual camera.

## Current implementation

- `ascii-cam-source` is the Media Foundation software-camera DLL. It provides an `IMFMediaSource`/`IMFMediaStream` and captures a physical webcam with a Source Reader.
- `ascii-cam` is a console test project for webcam capture and Media Foundation sample processing.
- The stream initializes the filter with `FilterOption::ASCII` and supports CPU and Direct3D 11/DXGI paths.
- The supported output formats are RGB32 and NV12. The default stream format is 1280x720 at 30 FPS.

The repository does not currently contain a GUI, an active OpenCV execution path, Named Shared Memory frame transport, or an HLSL shader implementation. `SharedMemory` and some OpenCV code remain as inactive experimental code.

## Requirements

- Windows 10 or later
- Visual Studio 2022
- Windows 10/11 SDK and the Desktop development with C++ workload
- A system with Media Foundation, Direct3D 11, Direct2D, and DirectWrite support

## Build

Open each solution in Visual Studio and build the `Release | x64` configuration.

1. Build `ascii-cam-source/ascii-cam.sln` to produce the virtual-camera DLL.
2. Build `ascii-cam/ascii-cam/ascii-cam.sln` to produce the capture test executable.

These are separate projects. The Windows-specific code cannot be built in the current Linux environment.

## Register the DLL

Run `regsvr32` for the built DLL from an elevated Command Prompt:

```bat
regsvr32 path\to\ascii-cam.dll
```

To unregister it:

```bat
regsvr32 /u path\to\ascii-cam.dll
```

The DLL is registered under `HKLM`, so registration and unregistration require administrator privileges. After registration, select the camera from applications that use Media Foundation cameras, such as Discord or Zoom.

## Status

The core capture, ASCII filter, and software-camera registration paths are implemented. A user interface, installer, automated Windows tests, and validated performance targets are still pending.
