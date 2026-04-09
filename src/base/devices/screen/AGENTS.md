# Screen Capture Module — Sub-Agent Guide

Parent: [Root AGENTS.md](../../../../AGENTS.md)

## Module Identity

This is the screen capture module (`traa::base::devices::screen`), the largest and most complex module in the project. It provides cross-platform screen/window enumeration, capture, and snapshot functionality.

CMake target: `screen` (OBJECT library), alias `traa::base::devices::screen`

## Directory Structure

```
src/base/devices/screen/
├── darwin/              # macOS implementations (.mm/.cc)
│   ├── screen_capturer_mac.h/mm      # CoreGraphics screen capturer
│   ├── screen_capturer_sck.h/mm      # ScreenCaptureKit capturer (macOS 12.3+)
│   ├── screen_capturer_darwin.mm      # Factory: create_raw_screen_capturer
│   ├── window_capturer_mac.mm         # Window capturer + factory
│   ├── enumerator_darwin.mm           # Screen/window enumeration + snapshot
│   ├── desktop_configuration*.h/mm    # Display configuration monitoring
│   ├── desktop_frame_cgimage.h/mm     # CGImage frame wrapper
│   ├── desktop_frame_iosurface.h/mm   # IOSurface frame wrapper
│   ├── desktop_frame_provider.h/mm    # Frame provider abstraction
│   ├── window_list_utils.h/cc         # Window enumeration helpers
│   └── window_finder_mac.h/mm         # Window-under-point detection
├── linux/               # Linux implementations
│   ├── capture_utils.h/cc             # Wayland detection
│   ├── enumerator_linux.cc            # X11 source enumeration (snapshot NOT_IMPLEMENTED)
│   └── x11/                           # X11-specific code
│       ├── shared_x_display.h/cc      # X Display connection management
│       ├── x_atom_cache.h/cc          # X11 atom caching
│       ├── x_error_trap.h/cc          # X11 error handling
│       ├── x_server_pixel_buffer.h/cc # Pixel buffer from X server
│       ├── x_window_list_utils.h/cc   # Window enumeration via X11
│       └── x_window_property.h/cc     # X11 window property access
├── win/                 # Windows implementations
│   ├── dxgi/            # DXGI Desktop Duplication API
│   ├── wgc/             # Windows Graphics Capture API
│   ├── screen_capturer_win_gdi.h/cc   # GDI screen capturer
│   ├── screen_capturer_win_directx.h/cc # DirectX screen capturer
│   ├── screen_capturer_win.cc         # Factory: create_raw_screen_capturer
│   ├── window_capturer_win_gdi.h/cc   # GDI window capturer
│   ├── window_capturer_win.cc         # Factory: create_raw_window_capturer
│   ├── enumerator_win.cc             # Screen/window enumeration + snapshot
│   ├── cropping_window_capturer*.h/cc # Screen-crop-based window capture
│   └── ...                            # Cursor, D3D, display config, etc.
├── test/                # Test utilities
│   ├── fake_desktop_capturer.h/cc
│   ├── mock_desktop_capturer_callback.h/cc
│   ├── desktop_frame_generator.h/cc
│   ├── screen_drawer*.h/cc           # Platform-specific screen drawing for tests
│   └── simple_window/                # Test window creation per platform
├── desktop_capturer.h/cc             # Abstract base class + factory methods
├── desktop_capture_options.h/cc      # Capture configuration
├── desktop_frame.h/cc                # Frame data structure
├── desktop_geometry.h/cc             # Rect, Vector, Size types
├── desktop_region.h/cc               # Region (set of rects)
├── enumerator.h/cc                   # screen_source_info_enumerator interface
├── desktop_capturer_differ_wrapper.h/cc  # Diff detection wrapper
├── blank_detector_desktop_capturer_wrapper.h/cc  # Blank frame detection
├── fallback_desktop_capturer_wrapper.h/cc  # Fallback chain wrapper
└── *_unittest.cc                     # Unit tests (co-located)
```

## Key Abstractions

### `desktop_capturer` (desktop_capturer.h)
Abstract base class for all screen/window capturers.
- `start(capture_callback*)` — Begin capturing
- `capture_frame()` — Capture one frame
- `get_source_list(source_list_t*)` — List available sources
- `select_source(source_id_t)` — Select capture target
- Factory methods: `create_window_capturer()`, `create_screen_capturer()`, `create_generic_capturer()`
- Protected: `create_raw_window_capturer()`, `create_raw_screen_capturer()` — Platform-specific factories

### `screen_source_info_enumerator` (enumerator.h)
Static utility class for the public API layer.
- `enum_screen_source_info()` — Enumerate screens and windows with thumbnails/icons
- `free_screen_source_info()` — Free enumerated data
- `create_snapshot()` — Create a scaled snapshot of a source
- `free_snapshot()` — Free snapshot data
- Each platform implements `enum_screen_source_info` and `create_snapshot` in its own `enumerator_*.cc/mm`

### `desktop_capture_options` (desktop_capture_options.h)
Configuration object passed to capturer factories. Platform-specific options:
- Windows: `allow_directx_capturer`, `allow_wgc_screen_capturer`, `allow_cropping_window_capturer`
- macOS: `allow_iosurface`, `allow_sck_capturer`, `configuration_monitor`
- Linux: `allow_pipewire` (Wayland), `x_display` (X11)

### Wrapper Pattern
Capturers can be wrapped for additional functionality:
- `desktop_capturer_differ_wrapper` — Computes updated regions by diffing frames
- `blank_detector_desktop_capturer_wrapper` — Detects and reports blank frames
- `fallback_desktop_capturer_wrapper` — Falls back to secondary capturer on failure

## Platform Implementation Status

| Platform | Enumeration | Screen Capture | Window Capture | Snapshot |
|----------|------------|----------------|----------------|----------|
| Windows  | ✅ enumerator_win.cc | ✅ GDI, DXGI, WGC | ✅ GDI, WGC | ✅ |
| macOS    | ✅ enumerator_darwin.mm | ✅ CoreGraphics, SCK | ✅ CGWindowList | ✅ |
| Linux/X11| ✅ enumerator_linux.cc | ❌ (null capturer) | ❌ (null capturer) | ❌ NOT_IMPLEMENTED |
| Wayland  | ❌ | 🔧 PipeWire (disabled) | 🔧 PipeWire (disabled) | ❌ |

## Adding a New Platform Capturer

1. Create platform directory (e.g., `android/`) under `src/base/devices/screen/`
2. Implement `desktop_capturer::create_raw_screen_capturer()` and `create_raw_window_capturer()`
3. Implement `screen_source_info_enumerator::enum_screen_source_info()` and `create_snapshot()`
4. Add files to `CMakeLists.txt` under the appropriate platform condition
5. Add unit tests as `*_unittest.cc`

## Adding a New Capture Backend (Same Platform)

1. Create a new class inheriting `desktop_capturer`
2. Implement `start()`, `capture_frame()`, `get_source_list()`, `select_source()`
3. Wire it into the platform's `create_raw_*_capturer()` factory, respecting `desktop_capture_options` flags
4. Consider adding a fallback wrapper if the new backend may fail

## Dependencies

- `desktop_geometry.h` — `desktop_rect`, `desktop_vector`, `desktop_size`
- `desktop_frame.h` — Frame data with stride, DPI, updated regions
- `desktop_region.h` — Region algebra (union, subtract, intersect)
- `libyuv` — Frame format conversion (used in enumerators for thumbnail scaling)
- Platform SDKs: CoreGraphics/ScreenCaptureKit (macOS), DXGI/D3D11/WGC (Windows), X11 (Linux)

## Testing

- Unit tests are co-located as `*_unittest.cc`
- Test utilities in `test/` directory (fake capturers, mock callbacks, frame generators)
- Platform-specific tests: `darwin/screen_capturer_mac_unittest.cc`, `win/wgc/*_unittest.cc`
- Integration test: `screen_capturer_integration_test.cc`
- Tests link against `traa::base::core` with `TRAA_UNIT_TEST` defined

## Critical Rules

1. Platform-specific code goes in platform subdirectories only
2. Use `#if defined(TRAA_OS_WINDOWS)` / `defined(TRAA_OS_MAC)` / `defined(TRAA_ENABLE_X11)` guards
3. No exceptions, no RTTI — use error codes and null returns
4. Memory: `new[]` in enumerators must be paired with `delete[]` in `free_*` functions
5. All capturers must handle `error_temporary` vs `error_permanent` correctly
6. Frame data uses BGRA pixel format (4 bytes per pixel)
7. **`desktop_frame` stride ≠ width × 4 on Windows** — DXGI and GDI capturers often produce frames where `stride()` is larger than `width() * 4` due to GPU texture alignment (e.g., 16-byte or 256-byte row alignment). Any code that consumes `desktop_frame::data()` must use `stride()` for row offsets, or strip the padding by copying row-by-row into a tightly-packed buffer before passing to APIs that assume `pitch == width * 4` (like `SDL_UpdateTexture`). Ignoring this causes diagonal tearing/shearing artifacts in rendered output.
