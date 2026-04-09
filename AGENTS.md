# AGENTS.md — traa Project Guide for AI Coding Assistants

## Project Identity

**traa** is a cross-platform C/C++ dynamic library for audio and video capture, processing, and display. It exposes a pure C API for easy integration. The project targets Windows, macOS, iOS, visionOS, Linux, and Android.

Repository: https://github.com/opentraa/traa

## Sub-Agent Hierarchy

This project uses a hierarchical agent structure. The root agent (this file) provides project-wide context. Each major module has its own `AGENTS.md` with deep implementation details.

```
AGENTS.md (this file)              ← Project-wide: API, build, conventions
├── src/base/AGENTS.md             ← Base library: module index, patterns, static lib structure
│   ├── src/base/thread/AGENTS.md  ← Threading: task_queue, futures, timers, weak callbacks
│   ├── src/base/devices/screen/AGENTS.md  ← Screen capture: capturers, enumerators, per-platform
│   └── src/base/devices/camera/   ← Camera capture: Windows DirectShow implemented, other platforms planned
├── src/main/AGENTS.md             ← Public API: engine, C wrappers, call flow, adding new APIs
└── examples/sdl_visual_demo/AGENTS.md ← SDL Demo: Panel architecture, UI rendering, build config (in progress)
```

When working on a specific module, read both the root AGENTS.md AND the relevant sub-agent file for full context.

## Architecture Overview

```
include/traa/           # Public C API headers (base.h, error.h, export.h, traa.h)
src/
├── base/               # Core library — platform abstractions, utilities [see src/base/AGENTS.md]
│   ├── base64/         # Base64 encoding/decoding
│   ├── devices/screen/ # Screen capture (largest module) [see src/base/devices/screen/AGENTS.md]
│   │   ├── darwin/     # macOS implementations (.mm/.cc)
│   │   ├── linux/      # Linux X11/Wayland implementations
│   │   │   └── x11/   # X11-specific code
│   │   ├── win/        # Windows implementations
│   │   │   ├── dxgi/   # DXGI capture
│   │   │   └── wgc/    # Windows Graphics Capture
│   │   └── test/       # Test utilities (fake capturers, mock callbacks)
│   ├── devices/camera/ # Camera capture [see .kiro/specs/windows-camera-capture/]
│   │   ├── win/        # Windows DirectShow (implemented)
│   │   ├── darwin/     # macOS/iOS AVFoundation (planned)
│   │   ├── linux/      # Linux V4L2 (planned)
│   │   └── android/    # Android Camera2 NDK (planned)
│   ├── folder/         # Filesystem utilities
│   ├── numerics/       # Numeric utilities (moving average, safe compare)
│   ├── sdk/            # SDK helpers
│   ├── strings/        # String utilities (Windows-specific)
│   ├── system/         # System utilities, metrics
│   ├── thread/         # Threading infrastructure [see src/base/thread/AGENTS.md]
│   └── utils/          # General utilities (time, etc.)
├── main/               # Public API implementation [see src/main/AGENTS.md]
│   ├── engine.h/cc     # Core engine — manages all subsystems
│   ├── traa.cc         # C API wrapper (routes calls through task queue)
│   ├── main.cc         # Shared library init/fini (constructor/destructor)
│   └── utils/          # Helper utilities (obj_string)
examples/
└── sdl_visual_demo/    # SDL3 visual demo [see examples/sdl_visual_demo/AGENTS.md]
    ├── panels/         # Panel implementations (device, screen_source, snapshot, camera, screen_capture, log)
    ├── ui/             # UI rendering utilities (theme, renderer)
    ├── utils/          # Frame converter, thread-safe frame buffer
    └── tests/          # Demo-specific unit and property tests
thirdparty/             # Git submodules (see Dependencies)
tests/
├── unit_test/          # Google Test unit tests
└── smoke_test/         # Integration/smoke tests
scripts/                # Build scripts (build.sh, build.bat)
cmake/                  # Toolchain files (linux/, macos/)
projects/android/       # Android Gradle project
```

## Build Targets

| CMake Target | Alias | Type | Description |
|---|---|---|---|
| `core` | `traa::base::core` | STATIC | Base library with all utilities |
| `main` | `traa::main` | SHARED | Final output library (`traa.dll`/`libtraa.so`/`traa.framework`) |
| `screen` | `traa::base::devices::screen` | OBJECT | Screen capture module |
| `camera` | `traa::base::devices::camera` | OBJECT | Camera capture module (cross-platform base + Windows DirectShow) |
| `unittest` | `traa::unittest` | EXECUTABLE | Unit tests |
| `smoketest` | `traa::smoketest` | EXECUTABLE | Smoke tests |
| `traa_sdl_demo` | — | EXECUTABLE | SDL3 visual demo app (gated by `TRAA_OPTION_ENABLE_SDL_DEMO`) |
| `traa_sdl_demo_test` | — | EXECUTABLE | SDL demo unit/property tests (gated by `TRAA_OPTION_ENABLE_UNIT_TEST` + `TRAA_OPTION_ENABLE_SDL_DEMO`) |

## Public C API

All public functions are in `include/traa/traa.h`, prefixed with `traa_`, and marked with `TRAA_API`:

```c
// Lifecycle
int  traa_init(const traa_config *config);
void traa_release();

// Events
int  traa_set_event_handler(const traa_event_handler *handler);

// Logging
void traa_set_log_level(traa_log_level level);
int  traa_set_log(const traa_log_config *config);

// Device enumeration
int  traa_enum_device_info(traa_device_type type, traa_device_info **infos, int *count);
int  traa_free_device_info(traa_device_info infos[]);

// Camera capture
int  traa_get_camera_capability(const char *device_id, traa_video_capability **capabilities, int *count);
int  traa_free_camera_capability(traa_video_capability *capabilities);
int  traa_start_camera_capture(const traa_camera_config *config);
int  traa_stop_camera_capture(const char *device_id);

// Screen capture (desktop platforms only)
int  traa_enum_screen_source_info(..., traa_screen_source_info **infos, int *count);
int  traa_free_screen_source_info(traa_screen_source_info infos[], int count);
int  traa_create_snapshot(int64_t source_id, traa_size snapshot_size, ...);
void traa_free_snapshot(uint8_t *data);
int  traa_start_screen_capture(const traa_screen_capture_config *config);
int  traa_stop_screen_capture(const int64_t source_id);
```

Error codes are defined in `include/traa/error.h` as `traa_error` enum (0 = success).

## Threading Model

- All public API calls are serialized through a main ASIO-based task queue (ID = 0).
- `task_queue_manager` (singleton) manages named task queues.
- The engine instance is `thread_local`, created/destroyed on the main queue thread.
- Timer classes: `task_timer_repeatly` (periodic), `task_timer_once` (one-shot).
- Async results use custom `ffuture` / `waitable_future`.

## Dependencies (Git Submodules in `thirdparty/`)

| Library | Purpose |
|---|---|
| **ASIO** | Async I/O, task scheduling (header-only, no exceptions) |
| **spdlog** | Logging (header-only, no exceptions) |
| **nlohmann/json** | JSON parsing (header-only) |
| **libyuv** | Video frame format conversion |
| **cpu_features** | CPU capability detection |
| **abseil-cpp** | Utility library (conditionally enabled) |
| **googletest** | Unit testing framework |
| **SDL3** | Window/rendering for SDL visual demo (conditionally included via `TRAA_OPTION_ENABLE_SDL_DEMO`) |

## Build System

### Requirements
- CMake ≥ 3.10, C++17
- Exceptions disabled (`-fno-exceptions` / `/EHs-c-`)
- RTTI disabled (`-fno-rtti` / `/GR-`)
- Symbols hidden by default (`-fvisibility=hidden`)
- Warnings as errors (`-Werror` / `/W4`)

### Build Commands

```bash
# Windows
scripts\build.bat -a x64 -t Release
scripts\build.bat -a x64 -t Debug -U     # with unit tests
scripts\build.bat -a x64 -t Debug --sdl-demo  # with SDL visual demo

# macOS
./scripts/build.sh -p macos -t Release
./scripts/build.sh -p macos --sdl-demo    # with SDL visual demo

# Linux
./scripts/build.sh -p linux -t Release -U ON

# Android
./scripts/build.sh -p android -A "arm64-v8a,x86_64"

# iOS
./scripts/build.sh -p ios -t Release
```

### Key CMake Options
- `TRAA_OPTION_ENABLE_UNIT_TEST` — Build unit tests (OFF)
- `TRAA_OPTION_ENABLE_SMOKE_TEST` — Build smoke tests (OFF)
- `TRAA_OPTION_NO_FRAMEWORK` — Skip framework on Apple (OFF)
- `TRAA_OPTION_ENABLE_X11` — Linux X11 support (ON)
- `TRAA_OPTION_ENABLE_WAYLAND` — Linux Wayland support (OFF)
- `TRAA_OPTION_ENABLE_SDL_DEMO` — Build SDL visual demo app (OFF, desktop only)
- `TRAA_OPTION_VERSION` — Version string (default "1.0.0")

## Coding Conventions

### Naming
- **Namespaces**: `traa::base`, `traa::main`
- **Classes/functions**: `snake_case` (e.g., `desktop_capturer`, `task_queue_manager`)
- **Macros**: `TRAA_UPPERCASE` (e.g., `TRAA_API`, `TRAA_ERROR_NONE`)
- **Constants**: `k_` prefix (e.g., `k_window_id_null`, `k_main_queue_name`)
- **Public C API**: `traa_` prefix + `snake_case`
- **Header guards**: `TRAA_` + path-based (e.g., `TRAA_BASE_THREAD_TASK_QUEUE_H_`)
- **Test files**: `*_unittest.cc`

### Patterns
- **Singleton**: `STRICT_SINGLETON_DECLARE(TypeName)` / `SINGLETON_DECLARE(TypeName)` macros
- **No-copy**: `DISALLOW_COPY_AND_ASSIGN(TypeName)` macro
- **No-construct**: `DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)` macro
- **Weak callbacks**: `support_weak_callback` base class + `make_weak_callback()`
- **Factory**: Static `create_*` methods for platform-specific implementations
- **Memory**: Alloc/free pairs (`traa_enum_*` / `traa_free_*`)

### Logging
```cpp
LOG_DEBUG/INFO/WARN/ERROR/FATAL("format {}", args);
LOG_API_ARGS_N(arg1, arg2, ...);  // API call tracing (N = 0..6)
LOG_*_IF(condition, "format {}", args);
```

### Platform Conditionals
Desktop-only screen capture features use:
```cpp
#if (defined(_WIN32) || defined(__APPLE__) || defined(__linux__)) && !defined(__ANDROID__) && \
    (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE) && \
    (!defined(TARGET_OS_VISION) || !TARGET_OS_VISION)
```

### Code Style
- LLVM-based `.clang-format` in project root
- 100 column limit
- 2-space indentation
- Attach braces (K&R style)
- Pointer alignment: right (`int *p`)
- No tabs

## Testing

- **Framework**: Google Test + Google Mock
- **Unit tests**: Link against `traa::base::core` (static), defined with `TRAA_UNIT_TEST` macro
- **Smoke tests**: Link against `traa::main` (shared library)
- **Sanitizers**: Enabled on macOS and Linux (ASan)
- **Test discovery**: `gtest_discover_tests()` for CTest integration
- **Test file location**: Co-located `*_unittest.cc` files next to source, plus `test/` directories for test utilities

### Smoke Test Coverage

Smoke tests in `tests/smoke_test/src/` provide exhaustive coverage of all 16 public C API functions:

| Test File | Coverage |
|---|---|
| `traa_engine_test.cc` | Multi-thread init/release, log level, log config, screen source enum, snapshot (original) |
| `traa_lifecycle_test.cc` | init/release lifecycle: valid config, nullptr, double init, reinit, post-release API calls |
| `traa_event_handler_test.cc` | Event handler set/replace/clear, nullptr handler, pre-init call, userdata verification |
| `traa_log_test.cc` | All 7 log levels, pre-init log level, log config with valid/nullptr/nonexistent paths |
| `traa_device_enum_test.cc` | Camera/microphone/speaker enumeration, field validation, nullptr params, unknown type, free |
| `traa_camera_capture_test.cc` | Camera capability query, capture start/stop, frame receipt, duplicate start, error paths |
| `traa_screen_source_test.cc` | Screen source flags (ignore screen/window/minimized/current process), icon/thumbnail, nullptr params |
| `traa_snapshot_test.cc` | Snapshot with valid/invalid source IDs, nullptr params, zero size, free |
| `traa_error_consistency_test.cc` | Systematic error code validation: uninitialized calls, nullptr params, enum range, double init |
| `traa_screen_capture_test.cc` | Screen capture start/stop, nullptr/invalid params, duplicate start, frame receipt, permission skip |

New smoke test files are auto-discovered by `GLOB_RECURSE` — no CMakeLists.txt changes needed. On MSVC, re-run cmake configure after adding new files. Each test file that needs the initialized engine redefines the `traa_engine_test` fixture locally (Google Test requires fixture visibility in the same translation unit).

## Adding New Code

### New source file
1. Create the `.h`/`.cc` file in the appropriate `src/base/` or `src/main/` subdirectory
2. Add the file to the corresponding `CMakeLists.txt`
3. Follow existing naming and include patterns

### New platform-specific implementation
1. Place in the platform subdirectory (`darwin/`, `linux/`, `win/`)
2. Use appropriate `#if` guards or CMake conditions
3. Add to the platform-specific section in `CMakeLists.txt`

### New public API function
1. Declare in `include/traa/traa.h` with `TRAA_API` and `TRAA_CALL`
2. Add types to `include/traa/base.h` if needed
3. Add error codes to `include/traa/error.h` if needed
4. Implement in `src/main/engine.h/cc`
5. Add C wrapper in `src/main/traa.cc` (route through task queue for thread safety)

### New unit test
1. Create `*_unittest.cc` next to the source file being tested
2. Add to `tests/unit_test/CMakeLists.txt`
3. Use Google Test macros (`TEST`, `TEST_F`, `EXPECT_*`, `ASSERT_*`)

## Critical Rules

1. **No exceptions** — The project compiles with `-fno-exceptions`. Never use `try`/`catch`.
2. **No RTTI** — Compiled with `-fno-rtti`. Never use `dynamic_cast` or `typeid`.
3. **Thread safety** — Public API calls must go through the main task queue.
4. **Pure C API** — Public headers must be valid C. Use `#if defined(__cplusplus)` for C++ features.
5. **Symbol visibility** — Only `TRAA_API`-marked functions are exported. Everything else is hidden.
6. **Memory pairing** — Every `traa_enum_*` must have a corresponding `traa_free_*`.
7. **CMakeLists updates** — New source files must be added to the relevant `CMakeLists.txt`.
8. **README sync** — When feature status changes (new, completed, deprecated) or project structure changes significantly, check and update `README.md` (Platform Support table, Features sections, Architecture description).

## Build Gotchas

- **Git submodules must be initialized before building**: Run `git clone --recurse-submodules` or `git submodule update --init --recursive`. Some submodules (especially `cpu_features`, `asio`) may silently appear as empty directories with only a `.git` file — use `git submodule update --init --force <path>` to fix.
- **Windows build.bat `-U` flag**: The `-U` (unittest) flag is a boolean toggle, not a key-value pair. Use `scripts\build.bat -a x64 -t Debug -U`, NOT `-U ON`. Same pattern applies to `--sdl-demo`.
- **DXGI warnings in tests are expected**: On machines without dedicated GPU or in remote desktop sessions, DXGI tests log "Cannot initialize any DxgiOutputDuplicator instance" warnings but still pass — this is by design (fallback behavior).
- **WebRTC-ported camera classes may have `protected` destructors**: WebRTC uses ref-counting (`AddRef`/`Release`) which requires protected destructors. traa uses `unique_ptr`/raw `new`/`delete`, so destructors must be `public`. When porting new WebRTC classes, check destructor visibility — `video_capture_ds::~video_capture_ds` needed this fix.
- **GLOB_RECURSE and MSVC**: The SDL demo CMakeLists uses `file(GLOB_RECURSE ...)` to collect source files. On MSVC, adding new `.cc`/`.h` files requires a cmake re-configure (`cmake -B build/win/x64 ...`) before rebuilding — the Visual Studio generator caches the file list and won't detect new files on a plain `cmake --build`.

## Current Status

### Implemented
- ASIO-based async threading model and task queues
- Screen source enumeration (Windows, macOS, Linux/X11)
- Screen capture on Windows (GDI, DXGI, Magnifier, WGC)
- Screen capture on macOS (CoreGraphics, ScreenCaptureKit, IOSurface)
- Window capture on macOS (CGWindowListCreateImage)
- Screen snapshot creation (Windows, macOS)
- Screen/window capturer abstract framework with differ wrapper, blank detection, fallback
- Logging system (spdlog)
- Cross-platform device abstraction layer
- Windows Camera Capture — DirectShow-based camera capture ported from WebRTC to `src/base/devices/camera/`. OBJECT lib `traa::base::devices::camera`. Cross-platform base classes (`video_capture_impl`, `device_info_impl`, `video_type` enum, `video_capture_capability`) plus Windows DirectShow implementation (`help_functions_ds`, `device_info_ds`, `sink_filter_ds`, `video_capture_ds`, factory functions). 10 property-based tests. Windows link deps: strmiids.lib, ole32.lib, oleaut32.lib.
- Camera Capture Integration — integrating camera module into engine and public C API. Spec at `.kiro/specs/camera-capture-integration/`. New public APIs: `traa_get_camera_capability`, `traa_free_camera_capability`, `traa_start_camera_capture`, `traa_stop_camera_capture`. New C types in `base.h`: `traa_video_frame`, `traa_video_capability`, `traa_video_frame_format`, `traa_camera_config`. Engine manages multiple simultaneous camera captures keyed by device_id. Frame callback (`on_video_frame`) runs on capture thread, not main queue.
- Continuous Screen Capture — `traa_start_screen_capture` / `traa_stop_screen_capture` public C API with `traa_screen_capture_config`. Engine manages per-session capture threads keyed by `int64_t source_id`, BGRA frame callback via `desktop_capturer`, optional `libyuv::ARGBScale` scaling. New `TRAA_VIDEO_FRAME_FORMAT_BGRA` enum value. SDL demo Screen Capture panel with real-time preview. 4 property-based tests + 7 smoke tests. Spec at `.kiro/specs/screen-capture-preview/`.

### Partially Implemented
- Screen capture on Linux — source enumeration via X11 works; snapshot (`create_snapshot`) returns `TRAA_ERROR_NOT_IMPLEMENTED`; raw screen/window capturers fall through to null capturer unless Wayland PipeWire is enabled

### Not Yet Implemented
- Camera capture on macOS (AVFoundation), Linux (V4L2), Android (Camera2 NDK)
- Audio Device Management (ADM) — microphone/speaker enumeration, capture, routing
- Audio/video stream processing — resampling, encoding, merging, streaming
- Screen capture on Android, iOS
- Wayland display server support (PipeWire capturer code exists but is gated behind `TRAA_ENABLE_WAYLAND`, which defaults to OFF)
- Linux screen snapshot creation
- AI-enhanced audio/video processing
