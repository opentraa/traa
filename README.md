# traa

[![Build Status](https://github.com/opentraa/traa/actions/workflows/ci-pr-on-main.yml/badge.svg)](https://github.com/opentraa/traa/actions)
![GitHub stars](https://img.shields.io/github/stars/opentraa/traa)
![GitHub forks](https://img.shields.io/github/forks/opentraa/traa)
![GitHub repo size](https://img.shields.io/github/repo-size/opentraa/traa)
![GitHub contributors](https://img.shields.io/github/contributors/opentraa/traa)
![GitHub last commit](https://img.shields.io/github/last-commit/opentraa/traa)

## Introduction

**traa** (Track Record Anything Anywhere) is a cross-platform C/C++ dynamic library for audio and video capture, processing, and display. It exposes a clean, pure C API so developers can easily integrate recording capabilities into any project.

## Vision

Create a very small but feature-rich dynamic library that covers the full audio/video pipeline — capture, processing, and display — across all major platforms. In the future, we aim to incorporate AI capabilities to enhance audio and video processing.

## Platform Support

| Platform | Architecture | Screen Enumeration | Screen Capture | Snapshot |
|----------|-------------|-------------------|----------------|----------|
| Windows  | x86, x64, ARM64 | ✅ | ✅ GDI, DXGI, WGC | ✅ |
| macOS    | Universal (x86_64 + arm64) | ✅ | ✅ CoreGraphics, ScreenCaptureKit | ✅ |
| Linux    | x86_64, aarch64 | ✅ X11 | 🔧 Partial | ❌ |
| Android  | arm64-v8a, armeabi-v7a, x86, x86_64 | ❌ | ❌ | ❌ |
| iOS      | arm64 | ❌ | ❌ | ❌ |
| visionOS | arm64 | ❌ | ❌ | ❌ |

## Features

### Implemented

- **Screen Source Enumeration** — Enumerate screens and windows with icon/thumbnail support (Windows, macOS, Linux/X11)
- **Screen Capture on Windows** — Multiple backends: GDI, DXGI, Windows Graphics Capture (WGC), with automatic fallback
- **Screen Capture on macOS** — CoreGraphics and ScreenCaptureKit (macOS 12.3+), with IOSurface acceleration
- **Window Capture on macOS** — CGWindowListCreateImage with full-screen window detection
- **Screen Snapshot** — Create scaled snapshots of any screen or window (Windows, macOS)
- **ASIO-based Async Threading** — Task queues with periodic/one-shot timers using `asio::io_context`
- **Capturer Framework** — Abstract `desktop_capturer` with differ wrapper, blank detection, and fallback wrappers

### In Progress

- **Linux Screen Capture** — X11 source enumeration works; raw capture and snapshot not yet implemented; Wayland PipeWire capturer exists but is disabled by default
- **Video Device Management (VDM)** — Windows camera capture via DirectShow (porting from WebRTC `modules/video_capture`); other platforms planned

### Planned

- **Audio Device Management (ADM)** — Enumeration, capture, and routing of microphones and speakers
- **Audio/Video Stream Processing** — Resampling, compression, encoding, merging, storage, voice changing, beautification, and streaming
- **Screen Capture on Android/iOS** — Mobile platform screen recording
- **Wayland Support** — PipeWire-based capture for modern Linux desktops
- **AI-Enhanced Processing** — Intelligent audio and video processing

## How to Build

### Prerequisites

- CMake 3.10+
- C++17 compiler
- **Windows**: Visual Studio Build Tools
- **macOS**: Xcode and Command Line Tools
- **Linux**: gcc/g++ or clang/clang++ (X11 dev packages for screen capture: `libx11-dev libxext-dev libxcomposite-dev libxrandr-dev`)
- **Android**: Android NDK
- **iOS/visionOS**: Xcode

### Clone

```sh
git clone --recurse-submodules https://github.com/opentraa/traa.git
cd traa
```

### Build

#### macOS
```sh
./scripts/build.sh -p macos -t Release
```

#### Windows
```batch
scripts\build.bat -a x64 -t Release
```

#### Linux
```sh
./scripts/build.sh -p linux -t Release
```

#### Android
```sh
./scripts/build.sh -p android -A "arm64-v8a,x86_64"
```

#### iOS
```sh
./scripts/build.sh -p ios -t Release
```

#### With Unit Tests
```sh
# macOS/Linux
./scripts/build.sh -p macos -U ON

# Windows
scripts\build.bat -a x64 -U ON
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `-p, --platform` | Target platform (macOS/iOS/xrOS/Linux/Android) | Required |
| `-t, --build-type` | Debug or Release | Release |
| `-a, --arch` | Architecture (Windows: Win32/x64/ARM64, Linux: x86_64/aarch64_clang/aarch64_gnu) | x64 / x86_64 |
| `-U, --unittest` | Build unit tests | OFF |
| `-S, --smoketest` | Build smoke tests | OFF |
| `-A, --android-abi` | Android ABIs | arm64-v8a,armeabi-v7a,x86,x86_64 |
| `-V, --verbose` | Verbose build output | OFF |
| `-v, --version` | Version string | 1.0.0 |

## Architecture

```
include/traa/       Public C API (base.h, error.h, export.h, traa.h)
src/base/           Core library — threading, screen capture, utilities
src/main/           API implementation layer (engine + C wrappers)
thirdparty/         Dependencies (ASIO, spdlog, libyuv, googletest, ...)
tests/              Unit tests and smoke tests
```

The library is built as a shared library (`traa.dll` / `libtraa.so` / `traa.framework`). All public API calls are thread-safe, serialized through an ASIO-based task queue.

## Contributing

We welcome contributions from the community. Feel free to open issues or submit pull requests to help improve traa.

## Support

If you find this project useful, a ⭐ on GitHub would be greatly appreciated. Your support motivates us to keep improving and adding new features.

## License

See [LICENSE](LICENSE) for details.
