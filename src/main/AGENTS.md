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

### `obj_string` (utils/obj_string.h)

Debug utility for converting traa types to JSON-like strings for `LOG_API_ARGS_N()` macros. Supports: `traa_config`, `traa_log_config`, `traa_event_handler`, `traa_size`, `traa_point`, `traa_rect`, `traa_device_type`, `traa_log_level`, `traa_video_frame_format`, `traa_video_capability`, `traa_video_frame`, `traa_camera_config`, pointers.

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

## Critical Rules

1. All public API calls MUST go through the main task queue (except stateless log functions)
2. The engine instance is `thread_local` — never access it from other threads
3. Always validate arguments before posting to the queue
4. Use `waitable_future::get(default_error)` — the default error handles queue-not-found
5. `TRAA_API` + `TRAA_CALL` on every exported function
6. Keep `traa.cc` thin — delegate real work to `engine` or base modules
7. Windows: `version.rc` is auto-generated from `resources/version.rc.in`
