# traa Module Map

This document summarizes the current module layout of `traa`, what each part is responsible for,
and how the pieces fit together today.

## Top-Level Layout

```text
traa/
├── include/        # Public C API and exported data structures
├── src/main/       # Shared-library entry points and API dispatch
├── src/base/       # Reusable internal utilities and platform implementations
├── tests/          # Unit tests and public API smoke tests
├── scripts/        # Cross-platform build entry points
├── cmake/          # Toolchains and CMake helpers
├── projects/       # Platform packaging projects, currently Android
├── thirdparty/     # Vendored dependencies
└── docs/           # Project notes and reference documents
```

## Layered View

```text
Public API
  include/traa/*.h
      |
      v
Shared Library Entry
  src/main/traa.cc
  src/main/engine.cc
      |
      v
Core Runtime + Feature Modules
  src/base/thread
  src/base/system
  src/base/devices/screen
  src/base/...
      |
      v
Platform Implementations
  win/
  darwin/
  linux/
```

## Public API Surface

### `include/traa/`

- `traa.h`
  The main exported API. Today it exposes library lifecycle, logging, device enumeration, screen
  source enumeration, and snapshot creation.
- `base.h`
  Shared public types such as `traa_size`, `traa_rect`, device types, screen-source metadata, and
  callback structs.
- `error.h`
  Public error codes. This already includes screen-specific errors such as permission denied,
  enumeration failure, and invalid source id.
- `export.h`
  Export macros and calling-convention definitions for the shared library.

### Current Public Capability Status

- `traa_init` / `traa_release`
  Implemented and routed through the internal task queue runtime.
- `traa_set_log_level` / `traa_set_log`
  Implemented.
- `traa_enum_device_info` / `traa_free_device_info`
  Exported but effectively placeholder behavior at the engine layer today.
- `traa_enum_screen_source_info` / `traa_free_screen_source_info`
  Implemented on Windows and macOS, partially implemented on Linux.
- `traa_create_snapshot` / `traa_free_snapshot`
  Implemented on Windows and macOS, not implemented on Linux.

## `src/main/`: Shared-Library Entry Layer

This layer is the narrow bridge between the exported C API and the internal C++ implementation.

### `src/main/traa.cc`

This is the real public API dispatcher:

- validates input
- initializes the shared runtime queue
- owns the `engine` instance lifetime through the task queue
- forwards API calls into `engine`

Important detail:

- lifecycle and device APIs go through the task queue
- screen-source enumeration and snapshot APIs currently call static engine helpers directly

### `src/main/engine.h` and `src/main/engine.cc`

This is intended to be the high-level coordinator for library behavior.

Current reality:

- screen enumeration and snapshot are wired through to `base::screen_source_info_enumerator`
- `init`, `set_event_handler`, `enum_device_info`, and `free_device_info` are still stubbed out

So `engine` exists as the right abstraction boundary, but only part of the planned responsibility
has been filled in.

### `src/main/main.cc`

Library load/unload hooks.

This file intentionally does very little. It sets up constructor/destructor behavior for the shared
library and avoids complex work during module load/unload.

### `src/main/utils/obj_string.h`

Small helper used for API argument logging and debug formatting.

## `src/base/`: Internal Foundation

`src/base/` is the internal toolkit that the rest of the project builds on.

### Core utility modules

- `base64/`
  Base64 helpers.
- `folder/`
  Cross-platform filesystem folder helpers.
- `numerics/`
  Numeric utilities such as safe compare and moving average.
- `system/`
  CPU detection, timing, sleep, metrics.
- `thread/`
  Futures, callbacks, task queue, thread utilities. This is one of the most important internal
  runtime modules because `traa_init` and several API calls depend on it.
- `utils/`
  Miscellaneous utility helpers, including time helpers and Windows-specific wrappers.
- top-level files such as `logger.*`, `checks.*`, `platform_thread.*`, `random.*`
  Shared foundational services used throughout the project.

### `base/devices/`

This namespace is where capture-device-related features are expected to live.

Current state:

- only `screen/` is wired in for non-mobile targets
- there is not yet a parallel camera/audio implementation tree with equivalent maturity

## `src/base/devices/screen/`: Most Mature Feature Area

This is currently the deepest and most complete subsystem in the repository.

### Shared screen-capture infrastructure

Representative responsibilities in the platform-agnostic files:

- desktop-frame representation and geometry
- dirty-region tracking and differ logic
- capture wrappers and fallbacks
- screen/window capturer abstractions
- source enumeration interface
- test utilities for capture flows

This area clearly borrows ideas and structure from mature desktop-capture codebases and has much
more implementation depth than the rest of `traa` today.

### `enumerator.h` / `enumerator.cc`

Platform-neutral interface for:

- enumerate screen/window sources
- free source lists
- create snapshots
- free snapshot buffers

This is the abstraction that `src/main/engine.cc` currently uses.

### Windows implementation

Located under `src/base/devices/screen/win/`.

Main parts:

- `enumerator_win.cc`
  Screen/window source enumeration and snapshot entry points.
- `dxgi/`
  Desktop duplication path.
- `wgc/`
  Windows Graphics Capture path.
- `screen_capturer_win_directx.*`, `screen_capturer_win_gdi.*`
  Capture backends.
- `window_capturer_win.*`
  Window capture support.

Status:

- enumeration is implemented
- snapshot creation is implemented
- backend coverage is broad compared with the rest of the project

### macOS implementation

Located under `src/base/devices/screen/darwin/`.

Main parts:

- `enumerator_darwin.mm`
  Source enumeration and snapshot creation.
- `screen_capturer_mac.*`
  Traditional macOS capture path.
- `screen_capturer_sck.*`
  ScreenCaptureKit-based path.
- `window_capturer_mac.mm`
  Window capture support.
- desktop configuration and frame-provider helpers

Status:

- enumeration is implemented
- snapshot creation is implemented
- permission checks are built into enumeration/snapshot flow
- full capture stack exists in source, though README had not reflected that yet

### Linux implementation

Located under `src/base/devices/screen/linux/`.

Main parts:

- `enumerator_linux.cc`
  Linux source enumeration entry.
- `x11/`
  X11-based source discovery and helpers.
- `capture_utils.*`
  Environment checks such as Wayland detection.

Status:

- source enumeration works only through the X11 path
- if running under Wayland, enumeration currently fails with
  `TRAA_ERROR_ENUM_SCREEN_SOURCE_INFO_FAILED`
- snapshot creation currently returns `TRAA_ERROR_NOT_IMPLEMENTED`

### Mobile targets

There is no equivalent implemented `screen/` feature path for Android, iOS, or xrOS in the same
way there is for Windows/macOS desktop capture. Mobile build targets exist, but feature
implementation is not at the same level.

## Tests

### `tests/unit_test/`

Primarily validates internal modules:

- numerics
- thread primitives
- system helpers
- many screen-capture support classes

This is where most low-level regression protection lives.

### `tests/smoke_test/`

Validates the exported library surface from a consumer point of view:

- repeated multi-threaded init/release
- logging APIs
- screen-source enumeration
- snapshot creation on supported desktop targets

This is the best place to understand what the project currently considers public, externally visible
behavior.

## Build and Packaging

### `scripts/`

- `build.sh`
  Main entry for macOS, iOS, xrOS, Linux, and Android.
- `build.bat`
  Main entry for Windows.

These scripts define the supported build matrix and are the practical entry point for users.

### `cmake/`

- shared configuration helpers
- Linux toolchain files
- Apple toolchain support

### `projects/android/`

Android packaging scaffold.

Current state:

- Gradle project exists
- AAR/JAR packaging flow exists in the build script
- Java wrapper is still demo-level and does not yet expose the real native surface cleanly

## Third-Party Dependencies

The repository vendors a number of dependencies under `thirdparty/`, including:

- `asio`
- `spdlog`
- `googletest`
- `cpu_features`
- `libyuv-static`
- `nlohmann/json`

These support the current runtime, logging, testing, CPU feature detection, and pixel processing
paths.

## Practical Reading Order

If you are new to the codebase, this order gives the quickest understanding:

1. `README.md`
2. `include/traa/traa.h`
3. `src/main/traa.cc`
4. `src/main/engine.cc`
5. `src/base/devices/screen/enumerator.h`
6. one platform enumerator:
   `win/enumerator_win.cc`, `darwin/enumerator_darwin.mm`, or `linux/enumerator_linux.cc`
7. `tests/smoke_test/src/traa_engine_test.cc`

## Current Architecture Summary

At the moment, `traa` is best understood as:

- a small exported C API
- backed by a growing C++ runtime layer
- with desktop screen capture as the most mature vertical slice
- and several planned modules still represented mainly by type definitions, stubs, and build
  scaffolding

That makes the project promising as a cross-platform capture SDK foundation, but still early in its
feature completion outside the desktop screen area.
