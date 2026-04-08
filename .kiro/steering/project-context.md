---
inclusion: always
---

# TRAA Project Context

## Project Overview

traa (Track Record Anything Anywhere) is a cross-platform dynamic library for audio/video capture, processing, and display. The goal is to build a small yet feature-rich C/C++ library that audio/video developers can easily integrate into their projects. Future plans include AI-enhanced audio/video processing.

Repository: https://github.com/opentraa/traa

## Public API

The public API is a pure C interface defined under `include/traa/`:

- `traa.h` — Main API entry point (init/release, device enumeration, screen capture, logging)
- `base.h` — Core type definitions (traa_size, traa_point, traa_rect, traa_device_info, traa_screen_source_info, traa_config, etc.)
- `error.h` — Error code enum (23 error types, TRAA_ERROR_NONE = 0)
- `export.h` — Platform export macros (TRAA_API, TRAA_CALL)

Key API functions:
- `traa_init()` / `traa_release()` — Lifecycle management
- `traa_set_event_handler()` — Event callbacks
- `traa_enum_device_info()` / `traa_free_device_info()` — Device enumeration
- `traa_enum_screen_source_info()` / `traa_free_screen_source_info()` — Screen source enumeration (desktop platforms only)
- `traa_create_snapshot()` / `traa_free_snapshot()` — Screen snapshots
- `traa_set_log_level()` / `traa_set_log()` — Log configuration

## Architecture

### Directory Structure

```
include/traa/          # Public C API headers
src/
├── base/              # Base library (platform abstractions, utilities)
│   ├── base64/        # Base64 encoding/decoding
│   ├── devices/       # Device management
│   │   ├── screen/    # Screen capture (core module)
│   │   │   ├── darwin/   # macOS/iOS implementation
│   │   │   ├── linux/    # Linux X11/Wayland implementation
│   │   │   └── win/      # Windows implementation (GDI, DXGI, Magnifier, WGC)
│   │   └── camera/    # Camera capture (see .kiro/specs/windows-camera-capture/)
│   │       ├── darwin/   # macOS/iOS AVFoundation (planned)
│   │       ├── linux/    # Linux V4L2 (planned)
│   │       ├── win/      # Windows DirectShow/MF (implemented)
│   │       └── android/  # Android Camera2 NDK (planned)
│   ├── folder/        # Filesystem utilities
│   ├── numerics/      # Numeric utilities
│   ├── sdk/           # SDK related
│   ├── strings/       # String utilities (Windows specific)
│   ├── system/        # System utilities
│   ├── thread/        # Threading infrastructure (task queues, futures)
│   └── utils/         # General utilities
├── main/              # Public API implementation layer
│   ├── engine.h/cc    # Core engine, manages all subsystems
│   ├── traa.cc        # C API wrapper functions
│   └── utils/         # Helper utilities
├── CMakeLists.txt
thirdparty/            # Third-party dependencies (git submodules)
scripts/               # Build scripts
cmake/                 # Toolchain files
projects/android/      # Android project
```

### Threading Model

- ASIO-based async threading model
- `task_queue_manager` manages multiple named task queues, main queue ID = 0
- All public API calls are serialized through the main task queue for thread safety
- `task_timer_repeatly` periodic timer / `task_timer_once` one-shot timer
- Custom `ffuture` and `waitable_future` for async results

### Screen Capture Architecture

- `desktop_capturer` abstract base class, each platform provides concrete implementations
- Windows: GDI (`screen_capturer_win_gdi`), DXGI (`screen_capturer_win_directx`), WGC (`wgc_capturer_win`), with fallback support
- macOS: CoreGraphics (`screen_capturer_mac`), ScreenCaptureKit (`screen_capturer_sck`, macOS 12.3+), auto-selects best capture method
- macOS window capture: `window_capturer_mac`, supports fullscreen window detection
- Linux: X11 screen source enumeration implemented; Wayland PipeWire capturer code exists but disabled by default
- `screen_source_info_enumerator` enumerates screens and windows (per-platform implementations: `enumerator_win.cc`, `enumerator_darwin.mm`, `enumerator_linux.cc`)
- Desktop frame objects support rotation, cropping, diff detection
- Wrapper pattern: `desktop_capturer_differ_wrapper` (diff detection), `blank_detector_desktop_capturer_wrapper` (blank detection), `fallback_desktop_capturer_wrapper` (fallback)

### Engine Model

The `engine` class is the core entry point, exposed externally through C API wrappers in `traa.cc`. The engine instance is thread_local, created and destroyed on the main task queue thread.

## Build System

### CMake Configuration

- Minimum version: CMake 3.10
- C++ standard: C++17 (required)
- Exceptions disabled (`-fno-exceptions`)
- RTTI disabled (`-fno-rtti`)
- Symbols hidden by default (`-fvisibility=hidden`)
- Warnings as errors (`-Werror`)

### Supported Platforms

| Platform | Toolchain | Architectures |
|----------|-----------|---------------|
| Windows | Visual Studio | x86, x64, ARM64 |
| macOS | Xcode | Universal Binary |
| iOS | Xcode | OS64COMBINED |
| visionOS | Xcode | VISIONOSCOMBINED |
| Linux | GCC/Clang | x86_64, aarch64 |
| Android | NDK | arm64-v8a, armeabi-v7a, x86, x86_64 |

### Build Commands

```bash
# Unix/Linux/macOS
./scripts/build.sh -p <platform> [options]
# Platforms: macos, ios, xros, linux, android

# Windows
scripts\build.bat [options]
# Architectures: Win32, x64, ARM64
```

Common options: `-t Debug/Release`, `-U ON` (unit tests, build.sh only), `-U` (unit tests, build.bat is a flag without argument), `-S ON` (smoke tests)

Note: Git submodules must be initialized before building (`git submodule update --init --recursive`). Some submodules may require `--force` to re-fetch.

### CMake Options

- `TRAA_OPTION_ENABLE_UNIT_TEST` — Enable unit tests
- `TRAA_OPTION_ENABLE_SMOKE_TEST` — Enable smoke tests
- `TRAA_OPTION_NO_FRAMEWORK` — Don't build framework on Apple platforms
- `TRAA_OPTION_ENABLE_X11` — Linux X11 support (default ON)
- `TRAA_OPTION_ENABLE_WAYLAND` — Linux Wayland support (default OFF)

### SDL Visual Demo (In Progress)

The project includes an SDL3-based visual demo app at `examples/sdl_visual_demo/`, enabled via `TRAA_OPTION_ENABLE_SDL_DEMO` (default OFF). Spec at `.kiro/specs/sdl-visual-demo/`.

Key architecture decisions:
- **Panel plugin pattern**: `panel_manager` manages functional panels (device, screen_source, snapshot, camera, log) via `panel_base` abstract class
- **UI rendering**: Pure SDL3 built-in API (`SDL_RenderDebugText`, basic draw primitives), no external UI library or font files
- **Thread-safe frame buffer**: `frame_buffer` class uses `std::mutex` for camera capture thread → SDL render thread data transfer, with minimal lock hold time (copy under lock, texture creation outside lock)
- **Image conversion**: Lightweight I420→BGRA converter using BT.601 coefficients, avoids depending on libyuv from the demo
- **Build integration**: `traa_sdl_demo` links `traa::main` (shared lib) + `SDL3::SDL3`; output dir matches `TRAA_ARCHIVE_OUTPUT_DIRECTORY` so the demo can find the traa shared library at runtime
- **Test target**: `traa_sdl_demo_test` links `gtest` + `SDL3::SDL3` + `traa::main`; uses `GLOB_RECURSE` to auto-discover test files; gated by both `TRAA_OPTION_ENABLE_SDL_DEMO` and `TRAA_OPTION_ENABLE_UNIT_TEST`
- **Header guards**: `TRAA_SDL_DEMO_` prefix (e.g., `TRAA_SDL_DEMO_APP_H_`, `TRAA_SDL_DEMO_PANELS_PANEL_BASE_H_`)
- **Build gotcha**: Use `scripts\build.bat --sdl-demo` (Windows) or `./scripts/build.sh -p <platform> --sdl-demo` (macOS/Linux) to build with SDL demo enabled
- **Known bug (fixed)**: Camera panel's device/capability list items were unclickable because `on_render` and `on_event` each defined layout constants as local `constexpr`. Fixed by extracting to class-level `static constexpr` members (`k_padding`, `k_btn_w`, `k_btn_h`, `k_line_height`, `k_gap`, `k_item_h`). Spec at `.kiro/specs/smoke-test-and-demo-fix/`.

## Third-Party Dependencies

All dependencies are managed via git submodules under `thirdparty/`:

| Library | Purpose |
|---------|---------|
| spdlog | Logging (header-only, no exceptions) |
| nlohmann/json | JSON parsing (header-only) |
| libyuv | Video frame conversion and processing |
| ASIO | Async I/O (header-only, no exceptions) |
| cpu_features | CPU capability detection |
| abseil-cpp | Utility library (conditionally used) |
| googletest | Unit testing framework |
| SDL3 | Window/rendering for SDL visual demo (conditional, `TRAA_OPTION_ENABLE_SDL_DEMO`) |

## Coding Conventions

### Naming Conventions

- Namespaces: `traa::base`, `traa::main`
- Class names: snake_case (e.g., `desktop_capturer`, `task_queue_manager`)
- Function names: snake_case
- Macros: UPPERCASE + `TRAA_` prefix
- Constants: `k_` prefix (e.g., `k_window_id_null`)
- Public C API functions: `traa_` prefix + snake_case
- Header guards: `TRAA_` + path + `_H_` (e.g., `TRAA_BASE_THREAD_TASK_QUEUE_H_`)

### Design Patterns

- Singleton: Thread-safe singletons (`SINGLETON_DECLARE` / `STRICT_SINGLETON_DECLARE` macros)
- RAII: Resource Acquisition Is Initialization
- Factory: Static factory methods for platform-specific implementations
- Wrapper/Decorator: Capturer wrappers (blank detection, diff detection, fallback)
- Weak Callback: Weak callback references to prevent circular dependencies
- `DISALLOW_COPY_AND_ASSIGN` / `DISALLOW_IMPLICIT_CONSTRUCTORS` macros to prevent copying

### Logging

Uses spdlog, wrapped via macros:
- `LOG_DEBUG/INFO/WARN/ERROR/FATAL(...)` — Basic logging
- `LOG_API_ARGS_N(...)` — API call tracing
- `LOG_*_IF(CONDITION, ...)` — Conditional logging

### Platform Conditional Compilation

Desktop platform screen capture features use the following condition:
```cpp
#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) && \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) && \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
```

### Code Formatting

The project root has a `.clang-format` file. All code should follow that format configuration.

## Testing

- Unit tests: Google Test framework, file naming `*_unittest.cc`
- Smoke tests: Integration tests
- Unit tests disabled on mobile platforms
- Performance metrics collection enabled during testing

## Implemented Features

- ASIO async threading model and task queues
- Screen source enumeration (Windows, macOS, Linux/X11)
- Windows screen capture (GDI, DXGI, Magnifier, WGC)
- macOS screen capture (CoreGraphics, ScreenCaptureKit, IOSurface)
- macOS window capture (CGWindowListCreateImage)
- Screen snapshot creation (Windows, macOS)
- Screen/window capturer abstract framework (diff detection wrapper, blank detection, fallback)
- Task timers (periodic and one-shot)
- spdlog logging system
- Cross-platform device abstraction
- Windows camera capture (DirectShow): Ported from WebRTC to `src/base/devices/camera/` (OBJECT library `traa::base::devices::camera`). Cross-platform base classes (`video_capture_impl`, `device_info_impl`, `video_type` enum, `video_capture_capability`) + Windows DirectShow implementation (`help_functions_ds`, `device_info_ds`, `sink_filter_ds`, `video_capture_ds`, factory functions). Includes 10 property-based tests. Windows link libraries: strmiids.lib, ole32.lib, oleaut32.lib.
- Camera capture integration: Camera module integrated into engine and public C API. 4 new public API functions (`traa_get_camera_capability`, `traa_free_camera_capability`, `traa_start_camera_capture`, `traa_stop_camera_capture`). New C types (`traa_video_frame_format`, `traa_video_capability`, `traa_video_frame`, `traa_camera_config`). Engine manages multiple simultaneous captures keyed by device_id. Frame callback `on_video_frame` runs on the capture thread (not the main queue). Includes 5 property-based tests + 13 unit tests.

## Partially Implemented Features

- Linux screen capture: Screen source enumeration via X11 is implemented; `create_snapshot` returns `TRAA_ERROR_NOT_IMPLEMENTED`; raw screen/window capturers fall back to empty implementation in non-Wayland environments

## Not Yet Implemented

- Camera capture on other platforms: macOS (AVFoundation), Linux (V4L2), Android (Camera2 NDK)
- Audio Device Management (ADM): Speaker/microphone enumeration, capture, routing
- Audio/video stream processing: Resampling, compression, encoding, merging, storage, voice changing, beauty filters, streaming
- Android/iOS screen capture
- Wayland support (PipeWire capturer code exists but disabled by default `TRAA_OPTION_ENABLE_WAYLAND=OFF`)
- Linux screen snapshot creation

## Development Notes

1. All public API calls must be serialized through the main task queue
2. The project compiles with exceptions and RTTI disabled — do not use try/catch or dynamic_cast
3. Platform-specific code goes in the corresponding platform subdirectory (darwin/, linux/, win/)
4. Public API must remain pure C interface; internal implementation uses C++17
5. Memory management follows alloc/free pairing pattern (e.g., `traa_enum_*` / `traa_free_*`)
6. New source files must be added to the corresponding CMakeLists.txt
7. Symbols are hidden by default; only functions marked with `TRAA_API` are exported
8. When feature status changes (new, completed, deprecated) or project structure changes significantly, check if `README.md` needs updating (Platform Support table, Features sections, Architecture description, etc.) and update accordingly

## Sub-Module Agent Documentation

The project uses a hierarchical agent structure. Each sub-module has its own AGENTS.md providing deep context:

- `AGENTS.md` — Project-wide: API, build, conventions
- `src/base/AGENTS.md` — Base library: module index, design patterns, static library structure
- `src/base/thread/AGENTS.md` — Threading module: task_queue, futures, timers, weak callbacks
- `src/base/devices/screen/AGENTS.md` — Screen capture: capturers, enumerators, per-platform implementation details
- `src/main/AGENTS.md` — Public API: engine, C wrappers, call flow, adding new API guide
- `examples/sdl_visual_demo/AGENTS.md` — SDL Demo: Panel architecture, UI rendering, build config (in progress)

When working on a specific module, reference both the root AGENTS.md and the corresponding sub-module's AGENTS.md.
