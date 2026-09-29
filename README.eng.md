# ASCII-Cam

**Language:** [한국어](README.md) | [English](README.eng.md)

# ASCII-Cam is a C++ software-camera project built on Windows Media Foundation. It reads frames from a physical webcam, uses a Direct3D 11 compute shader to create an ASCII-style luminance pattern, and exposes a Media Foundation virtual-camera source.

This document describes the code that is currently present, separating the active runtime path from experimental components. It is intended to help new contributors build the projects and trace a frame through the system.

## Repository Layout

| Path | Responsibility |
| --- | --- |
| `ascii-cam-source/` | Media Foundation software-camera source DLL. Implements COM activation, `IMFMediaSource`/`IMFMediaStream`, webcam capture, and ASCII post-processing. |
| `ascii-cam/` | Console host/test project. It creates and starts a session-lifetime virtual camera using the DLL's CLSID. Capture-test code also remains in `main.cpp`. |
| `README.md`, `README.eng.md` | Korean and English technical overview and developer guide. |

Key implementation files:

| File | Responsibility |
| --- | --- |
| `ascii-cam-source/ascii-cam/dllmain.cpp` | DLL entry point, COM class factory, and CLSID registration/unregistration. |
| `ascii-cam-source/ascii-cam/Activator.cpp` | Activates the camera object requested by Media Foundation. |
| `ascii-cam-source/ascii-cam/MediaSource.cpp` | Manages source attributes, presentation descriptor, stream selection, and start/stop events. |
| `ascii-cam-source/ascii-cam/MediaStream.cpp` | Defines output media types; owns the capture thread, filter, latest-frame slot, and `MEMediaSample` delivery. |
| `ascii-cam-source/ascii-cam/WebCamCapture.cpp` | Enumerates physical cameras, selects a native type, and captures frames through a Source Reader. |
| `ascii-cam-source/ascii-cam/Filter.cpp` | Locks the sample buffer and calls the selected post-processing engine. |
| `ascii-cam-source/ascii-cam/AsciiEngine.cpp` | Creates D3D11 resources and a compute shader, then writes an ASCII luminance pattern into the NV12 Y plane. |
| `ascii-cam-source/ascii-cam/FrameGenerator.cpp` | Implements a separate Direct2D/WIC frame-generation and RGB32/NV12 conversion path. It is not called by the live `RequestSample` path. |

## Architecture and Frame Flow

```mermaid
flowchart LR
	Client[Media Foundation camera client] -->|MFCreateVirtualCamera / Start| Host[ascii-cam host]
	Host -->|Activate CLSID| Activator[Activator]
	Activator --> Source[MediaSource]
	Source --> Stream[MediaStream]
	Stream -->|Start| Capture[WebcamCapture / Source Reader]
	Capture -->|latest sample| Filter[Filter]
	Filter --> Ascii[AsciiEngine / D3D11 compute]
	Ascii -->|modified Y plane| Latest[latest-frame slot]
	Client -->|RequestSample| Stream
	Latest -->|copy sample and MEMediaSample| Client
```

1. The host calls `MFStartup`, then `MFCreateVirtualCamera`. The current implementation uses the name `Ascii Cam`, `MFVirtualCameraLifetime_Session`, `MFVirtualCameraAccess_CurrentUser`, and DLL CLSID `{3CAD447D-F283-4AF4-A3B2-6F5363309F52}`.
2. Media Foundation calls the DLL class factory. `Activator` creates a `MediaSource`, which currently exposes one video stream.
3. When the client starts the stream, `MediaStream::Start` initializes `WebcamCapture` with the selected dimensions and frame rate, initializes the ASCII filter, and starts the capture thread.
4. The capture thread reads frames from the Source Reader and passes each sample to `Filter::Process`. The newest successful sample is stored in `_latestFrame` behind a lock. This is a latest-frame design; old frames are not queued.
5. When the frame server calls `RequestSample`, the stream copies the current `_latestFrame`, assigns sample time/duration/token metadata, and sends it in a `MEMediaSample` event.
6. `Stop` and `Shutdown` are intended to release the stream, event queue, and capture resources. The current thread-stop issue is called out under [Known Limitations](#known-limitations-and-follow-up-work).

## Capture and Media Types

- `MediaStream` defaults to 1280x720 at 30/1 FPS. Its stream descriptor advertises RGB32 and NV12; the current code selects NV12 as the initial media type.
- `WebcamCapture` enumerates cameras, skips devices whose friendly name contains `Virtual`, and opens the first remaining device it can activate. There is no device-selection UI or camera-ID setting.
- Capture chooses the highest-frame-rate native mode matching the requested resolution. If no exact resolution is available, it falls back to a YUY2 native mode.
- If the selected native type is not YUY2, the Source Reader is asked to output NV12. If YUY2 is selected, that type is kept as-is.
- If creating the D3D11/DXGI connection for the Source Reader fails, reader creation is retried with CPU attributes. This fallback applies to capture decoding; it is not a software implementation of the ASCII engine.

## ASCII Filter

The active stream initializes the filter with `FilterOption::ASCII` in `MediaStream::Start`.

1. `Filter` locks the first Media Foundation buffer with `IMF2DBuffer::Lock2D` and passes its row pitch to the engine. If the buffer does not expose the 2D interface, it locks the linear buffer instead.
2. `AsciiEngine` creates a hardware D3D11 device, an R8 input texture, an RGBA output texture, a staging texture, and a compute shader. The embedded HLSL is compiled when the engine is initialized.
3. For each frame, the Y plane is copied to the GPU input texture and the compute shader runs. The shader computes average luminance in each 8x16-pixel cell, applies nonlinear tone mapping, and selects from 10 embedded glyph bitmaps.
4. The output is copied to CPU staging memory. The first output channel (black or white) overwrites the original buffer's Y plane. The NV12 UV plane is left unchanged, so the input frame's chroma data remains.

This is not a CPU ASCII-processing path. Even though webcam-reader setup has a CPU fallback, `AsciiEngine::Initialize` requires creation of a hardware D3D11 device.

## Build and Run

### Requirements

- Windows 10 or later with Media Foundation
- Visual Studio 2022, the Desktop development with C++ workload, and the Windows 10/11 SDK
- The v143 platform toolset and a C++20 compiler
- D3D11 hardware support
- Restore the Windows Implementation Library (WIL) NuGet package referenced by the source project

### Build

Open each solution in Visual Studio and select `Release | x64`.

1. Build `ascii-cam-source/ascii-cam.sln` to produce the Media Foundation camera-source DLL.
2. Build `ascii-cam/ascii-cam/ascii-cam.sln` to produce the console host that creates a session virtual camera.

These are separate projects. The host uses the source DLL's CLSID, so the DLL must be registered in Windows before the virtual camera is created. This repository is Windows-specific; its solutions cannot be built or its virtual camera run on Linux.

### Register the DLL and Start the Camera

From an elevated 64-bit Command Prompt, register the DLL. Replace `path\\to\\ascii-cam.dll` with the actual build output path.

```bat
%SystemRoot%\\System32\\regsvr32.exe path\\to\\ascii-cam.dll
```

`DllRegisterServer` registers the DLL CLSID under `HKLM\\Software\\Classes\\CLSID\\{...}\\InprocServer32`. To unregister:

```bat
%SystemRoot%\\System32\\regsvr32.exe /u path\\to\\ascii-cam.dll
```

Then run the console host. On success, it creates and starts the session camera named `Ascii Cam`, then waits for a key press. On exit it calls `Stop` and `Remove`. To inspect the device in another camera application, refresh its device list while the host is running. If the application cannot find the source DLL, verify the registration and CLSID.

### Diagnostics

- Host failures from `MFStartup`, `MFCreateVirtualCamera`, and `Start` print an HRESULT to the console.
- The DLL uses `WINTRACE`/WIL result logging. Inspect Visual Studio's Debug Output for device enumeration and Media Foundation errors.
- If the webcam cannot be opened, check whether another application has exclusive access and whether Windows camera permissions are enabled.
- If D3D11 or DXGI manager creation fails, capture setup can retry in CPU mode, but the ASCII filter still requires D3D11 hardware.

## Known Limitations and Follow-up Work

These are observations about the current code, not guarantees of intended behavior. Resolve or test these contracts before extending the feature set.

- **Advertised type versus sample mismatch:** The stream advertises RGB32/NV12, but `RequestSample` neither checks the requested type nor converts the sample. It copies the first buffer from `_latestFrame`, so the client can receive a buffer whose format differs from the advertised type. `FrameGenerator`'s conversion path is not connected to this request path.
- **YUY2 fallback versus filter input:** When YUY2 is selected, the filter still interprets the start of the sample as an NV12 Y plane. There is no unpacking path for YUY2's packed layout, so filter output may be incorrect.
- **Capture timestamps:** `RequestSample` does not forward the captured timestamp. It sets `MFGetSystemTime()` and a fixed duration of `333333` (100-ns units), which may not match camera cadence or synchronization.
- **First sample request:** If capture has not produced a frame yet, `RequestSample` returns `S_OK` without sending an event. No wait, retry, or failure-event policy is defined.
- **Capture-thread stop signal:** `Stop` joins the capture thread without requesting its stop token, and the loop's stream state is changed only after the join. Stop may therefore fail to return during normal capture. Fix the shutdown ordering and add a regression test.
- **Error propagation:** `Filter` does not check the HRESULT returned by `AsciiEngine::Process`, and some buffer-lock failure paths do not guarantee unlock. D3D failures should not be assumed to reach the stream client.
- **Scope:** There is no GUI, installer, device-selection UI, automated Windows integration test, or validated performance target. `ColorConverter` is marked deprecated, and `FrameGenerator` is not used by the live request path. `SharedMemory` and older OpenCV experiments are also outside the active camera path.

## Suggested Next Steps

1. Normalize capture input to one explicit internal format (for example NV12) and implement conversion for each advertised RGB32/NV12 output type.
2. Preserve source timestamps/durations and define first-frame waiting and device-disconnect event behavior.
3. Correct stop-token and state-transition ordering; add start/stop/restart and device-disconnect tests.
4. Make buffer locking and HRESULT propagation reliable, then define behavior when D3D11 hardware is unavailable.
5. Validate media negotiation, camera-client activation, and registration/unregistration in Windows CI or a developer checklist.
