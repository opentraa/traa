# Main (Public API) Module — Sub-Agent Guide

Parent: [Root AGENTS.md](../../AGENTS.md)

## Module Identity

This is the public API implementation module (`traa::main`). It builds the final shared library (`traa.dll` / `libtraa.so` / `traa.framework`) and is the only module that exports symbols.

CMake target: `main` (SHARED library), alias `traa::main`, output name `traa`

## Directory Structure

```
src/main/
├── engine.h/cc        # Core engine class — manages all subsystems
├── traa.cc            # C API wrappers — routes calls through task queue
├── main.cc            # Shared library init/fini (constructor/destructor attributes)
├── utils/
│   └── obj_string.h   # Debug string conversion for all traa types
└── CMakeLists.txt
```

## Architecture

### Call Flow

```
User code
  → traa_init(config)                    [traa.cc — C API]
    → task_queue_manager::post_task(0, lambda)  [serialize on main queue]
      → engine::init(config)             [engine.cc — on main queue thread]
        → return result via waitable_future
```

All public API functions follow this pattern:
1. Validate arguments (return error immediately if invalid)
2. Post work to main queue (ID = 0) via `task_queue_manager::post_task()`
3. Block on `waitable_future::get(default_error)` to return result

Exception: `traa_set_log_level()` and `traa_set_log()` are stateless and execute directly.

Exception: Screen capture functions (`traa_enum_screen_source_info`, `traa_create_snapshot`, etc.) call `engine` static methods directly (they are thread-safe on their own).

### `engine` Class (engine.h)

Central manager inheriting `support_weak_callback`. Responsibilities:
- `init(config)` — Initialize the engine
- `set_event_handler(handler)` — Register event callbacks
- `enum_device_info(type, infos, count)` — Enumerate devices
- `free_device_info(infos)` — Free device info
- Static methods (desktop platforms only):
  - `enum_screen_source_info()` — Delegates to `screen_source_info_enumerator`
  - `free_screen_source_info()` — Delegates to `screen_source_info_enumerator`
  - `create_snapshot()` / `free_snapshot()` — Delegates to `screen_source_info_enumerator`
- Non-static methods (desktop platforms only):
  - `start_screen_capture(config)` / `stop_screen_capture(source_id)` — Continuous screen capture

The engine instance is `thread_local` on the main queue thread. It is created in `traa_init()` and destroyed when the main queue is released.

### Camera Capture Integration (implemented)

Engine manages camera captures keyed by device_id (multiple simultaneous captures supported). Camera-related engine methods:
- `get_camera_capability(device_id, capabilities, count)` / `free_camera_capability(capabilities)`
- `start_camera_capture(config)` / `stop_camera_capture(device_id)`
- `ensure_camera_device_info()` — lazy-initializes `device_info_impl` via `base::create_device_info()`
- `enum_device_info(TRAA_DEVICE_TYPE_CAMERA)` delegates to platform `device_info_impl`

Private members: `camera_device_info_` (unique_ptr, lazy init), `camera_captures_` (unordered_map<string, capture_context>).

Key design decisions:
- `on_video_frame` callback runs on the capture thread (not main queue) for zero-copy performance
- `frame_callback_adapter` (anonymous namespace in engine.cc) bridges `video_frame_callback::on_frame` → user's C function pointer, always delivers I420 format
- Engine destructor stops all active captures, clears the map, resets device_info
- Factory functions `create_device_info()` and `create_video_capture()` are forward-declared in engine.cc (defined in platform-specific .cc files)

### `main.cc` — Library Lifecycle

Handles shared library load/unload:
- **GCC/Clang**: `__attribute__((constructor))` / `__attribute__((destructor))`
- **MSVC**: `.CRT$XCU` section for init, `atexit()` for cleanup
- Currently minimal — just debug logging, no heavy initialization

### Screen Capture Integration (implemented)

Engine manages continuous screen captures keyed by `int64_t source_id` (multiple simultaneous captures supported). Screen-capture-related engine methods (desktop platforms only, non-static):
- `start_screen_capture(config)` / `stop_screen_capture(source_id)`

Private members: `screen_captures_` (`unordered_map<int64_t, screen_capture_context>`), inside desktop platform guard.

Key design decisions and differences from camera capture:
- **Callback ownership**: `screen_capture_callback` is created as `unique_ptr`, then moved into a `shared_ptr` captured by the thread lambda. This is necessary because `desktop_capturer::start(callback*)` takes a raw pointer, so the callback must outlive both the capturer and the thread. The `shared_ptr` ensures the callback is destroyed only after the thread exits.
- **Thread model**: Each capture session spawns a dedicated `std::thread` that loops `capture_frame()` + `sleep_for(33ms)` (~30fps). This differs from camera capture which relies on the platform capture module's internal threading. The thread is joined on `stop_screen_capture()` or engine destruction.
- **Source type detection**: `start_screen_capture` calls `enum_screen_source_info()` at startup to determine if the `source_id` refers to a window or screen, then creates the appropriate capturer via `create_window_capturer()` or `create_screen_capturer()`. This incurs a one-time enumeration cost per start.
- **Frame format**: Always delivers BGRA (`TRAA_VIDEO_FRAME_FORMAT_BGRA`), matching `desktop_frame`'s native format. Optional scaling via `libyuv::ARGBScale` when `config.frame_size` is non-zero.
- **`screen_capture_context`**: Move-only struct (contains `atomic<bool>`, `unique_ptr`). Uses `DISALLOW_COPY_AND_ASSIGN`. Emplaced into the map via `piecewise_construct` to avoid copy/move issues with `std::atomic`.
- **Forward declaration in base.h**: `traa_screen_capture_config` references `traa_video_frame` in its function pointer type, but `traa_video_frame` is defined later in the file. A `struct traa_video_frame;` forward declaration was added before the config struct to resolve this.
- **Stride stripping (bug fix)**: `desktop_frame::stride()` on Windows is often larger than `width * 4` due to GPU texture alignment. Since `traa_video_frame` has no stride field, the callback must deliver tightly-packed BGRA data. When `stride != width * 4`, the no-scale path copies row-by-row into `scale_buffer_` to strip padding. Without this, rendered output shows diagonal tearing/shearing.

### `obj_string` (utils/obj_string.h)

Debug utility for converting traa types to JSON-like strings for `LOG_API_ARGS_N()` macros. Supports: `traa_config`, `traa_log_config`, `traa_event_handler`, `traa_size`, `traa_point`, `traa_rect`, `traa_device_type`, `traa_log_level`, `traa_video_frame_format`, `traa_video_capability`, `traa_video_frame`, `traa_camera_config`, `traa_screen_capture_config` (desktop only), pointers.

## Links to Base Module

- `traa::base::core` — Static library with all base functionality
- `traa::base::task_queue_manager` — Thread serialization
- `traa::base::screen_source_info_enumerator` — Screen capture delegation
- `traa::base::logger` — Logging configuration

## Links to Third-Party

- `yuv` (libyuv) — Frame format conversion
- `cpu_features` — CPU capability detection
- Platform libraries: `dwmapi`, `Shcore`, `dxgi`, `d3d11`, `Winmm` (Windows); `strmiids`, `ole32`, `oleaut32` (Windows, camera DirectShow); Apple frameworks (macOS/iOS)

## Adding a New Public API Function

1. Declare in `include/traa/traa.h` with `TRAA_API int TRAA_CALL traa_new_function(...);`
2. Add types/enums to `include/traa/base.h` if needed
3. Add error codes to `include/traa/error.h` if needed
4. Add method to `engine` class in `engine.h/cc`
5. Add C wrapper in `traa.cc`:
   ```cpp
   int traa_new_function(...) {
     LOG_API_ARGS_N(...);
     // validate args
     return traa::base::task_queue_manager::post_task(g_main_queue_id, [...]() {
       return g_engine_instance->new_function(...);
     }).get(traa_error::TRAA_ERROR_NOT_INITIALIZED);
   }
   ```
6. Add `obj_string::to_string()` overload for new types
7. Add smoke test in `tests/smoke_test/`

## Adding a New Subsystem (e.g., ADM, VDM)

1. Create implementation in `src/base/` (e.g., `src/base/devices/audio/`)
2. Add manager class to `engine` (e.g., `audio_device_manager`)
3. Add public API functions following the pattern above
4. The engine creates/destroys subsystem managers in `init()`/destructor
5. Create sub-agent AGENTS.md for the new subsystem

## API Parameter Validation Gotchas

These behaviors were discovered during exhaustive smoke testing and are important for writing correct tests and client code:

1. **`traa_enum_device_info`** — Only returns `TRAA_ERROR_INVALID_ARGUMENT` when BOTH `infos` AND `count` are nullptr. Passing only one as nullptr lets the call proceed to the engine (may crash or return unexpected results).
2. **`traa_get_camera_capability`** — Same pattern: only validates when BOTH `capabilities` AND `count` are nullptr. Single nullptr passes through.
3. **`traa_enum_screen_source_info`** — Same pattern: only validates when BOTH `infos` AND `count` are nullptr.
4. **`traa_set_log`** — Has NO init check. It directly configures the logger and returns `TRAA_ERROR_NONE` even before `traa_init()` is called. This is unlike most other APIs which return `TRAA_ERROR_NOT_INITIALIZED`.
5. **`traa_set_log_level`** — Stateless, can be called at any time (before/after init). This is documented and expected.
6. **`engine::init()`** — Always returns `TRAA_ERROR_NONE`. There is currently no double-init detection (`TRAA_ERROR_ALREADY_INITIALIZED` is never returned). Calling `traa_init` twice without `traa_release` succeeds silently.
7. **`traa_create_snapshot` with zero-size** — Returns `TRAA_ERROR_NOT_FOUND` (not `TRAA_ERROR_INVALID_ARGUMENT`) when snapshot_size is {0,0}.
8. **`traa_screen_capture_config` in base.h** — The struct's `on_video_frame` function pointer references `traa_video_frame`, which is defined later in the file. A `struct traa_video_frame;` forward declaration is required before the config struct. Without it, C compilers will error on the incomplete type in the function pointer parameter.

## Critical Rules

1. All public API calls MUST go through the main task queue (except stateless log functions)
2. The engine instance is `thread_local` — never access it from other threads
3. Always validate arguments before posting to the queue
4. Use `waitable_future::get(default_error)` — the default error handles queue-not-found
5. `TRAA_API` + `TRAA_CALL` on every exported function
6. Keep `traa.cc` thin — delegate real work to `engine` or base modules
7. Windows: `version.rc` is auto-generated from `resources/version.rc.in`
