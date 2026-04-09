# Base Library — Sub-Agent Guide

Parent: [Root AGENTS.md](../../AGENTS.md)

## Module Identity

This is the base library (`traa::base`), providing all platform abstractions, utilities, and core infrastructure. It compiles as a static library (`core.lib`/`libcore.a`).

CMake target: `core` (STATIC library), alias `traa::base::core`

## Sub-Modules

| Directory | Alias | Type | Description |
|-----------|-------|------|-------------|
| `base64/` | `traa::base::base64` | OBJECT | Base64 encoding/decoding |
| `devices/screen/` | `traa::base::devices::screen` | OBJECT | Screen capture (see [screen/AGENTS.md](devices/screen/AGENTS.md)) |
| `folder/` | `traa::base::folder` | OBJECT | Filesystem path utilities |
| `numerics/` | `traa::base::numerics` | OBJECT | Moving average, safe integer comparison |
| `strings/` | `traa::base::strings` | OBJECT | String conversion (Windows only) |
| `system/` | `traa::base::system` | OBJECT | CPU features, metrics, sleep, system time |
| `thread/` | `traa::base::thread` | OBJECT | Task queues, futures, callbacks (see [thread/AGENTS.md](thread/AGENTS.md)) |
| `utils/` | `traa::base::utils` | OBJECT | Time utilities, Windows COM/registry/version helpers |

## Root-Level Files (in src/base/)

| File | Purpose |
|------|---------|
| `arch.h` | Architecture detection macros (`TRAA_ARCH_X86_FAMILY`, `TRAA_ARCH_ARM_FAMILY`) |
| `arraysize.h` | `arraysize()` macro for C arrays |
| `checks.h/cc` | `TRAA_DCHECK`, `TRAA_CHECK` assertion macros |
| `disallow.h` | `DISALLOW_COPY_AND_ASSIGN`, `DISALLOW_IMPLICIT_CONSTRUCTORS` |
| `function_view.h` | Non-owning function reference (like `std::function_ref`) |
| `hedley.h` | Compiler feature detection (third-party header) |
| `logger.h/cc` | spdlog wrapper macros (`LOG_DEBUG/INFO/WARN/ERROR/FATAL`) |
| `platform.h` | Platform detection (`TRAA_OS_WINDOWS`, `TRAA_OS_MAC`, `TRAA_OS_LINUX`, etc.) |
| `platform_thread.h/cc` | `platform_thread` — RAII thread wrapper with name and priority |
| `random.h/cc` | `random` — Thread-safe random number generator |
| `singleton.h` | `SINGLETON_DECLARE` / `STRICT_SINGLETON_DECLARE` macros |
| `string_to_number.h/cc` | Safe string-to-number conversion |
| `string_utils.h/cc` | String utilities (hex conversion, compile-time strings) |
| `thread_annotations.h` | Thread safety annotation macros (`GUARDED_BY`, `ACQUIRED_AFTER`) |
| `type_traits.h` | Type trait helpers |
| `win32.h` | Windows-specific includes and defines |

## How the Static Library is Built

`core` aggregates all OBJECT libraries via `$<TARGET_OBJECTS:...>`:

```cmake
target_sources(core PRIVATE
    $<TARGET_OBJECTS:traa::base::base64>
    $<TARGET_OBJECTS:traa::base::folder>
    $<TARGET_OBJECTS:traa::base::numerics>
    $<TARGET_OBJECTS:traa::base::system>
    $<TARGET_OBJECTS:traa::base::thread>
    $<TARGET_OBJECTS:traa::base::utils>
    $<TARGET_OBJECTS:traa::base::json>      # nlohmann/json
    $<TARGET_OBJECTS:traa::base::asio>      # ASIO headers
    # Platform-specific:
    $<TARGET_OBJECTS:traa::base::strings>   # Windows only
    $<TARGET_OBJECTS:traa::base::devices::screen>  # Desktop platforms only
)
```

## Platform-Specific Code Locations

| Platform | Directories |
|----------|-------------|
| Windows | `strings/`, `utils/win/`, `devices/screen/win/` |
| macOS | `devices/screen/darwin/` |
| Linux | `devices/screen/linux/`, `devices/screen/linux/x11/` |
| All | Everything else |

## Adding a New Sub-Module

1. Create directory under `src/base/` (e.g., `src/base/audio/`)
2. Create `CMakeLists.txt` with OBJECT library target
3. Add `add_subdirectory()` in `src/base/CMakeLists.txt`
4. Add `$<TARGET_OBJECTS:...>` to `core` target sources
5. Create `AGENTS.md` in the new directory for sub-agent context

## Coding Patterns Used Throughout

- **snake_case** for all identifiers
- **Namespace**: `traa::base` (or `traa::base::sub_namespace` for larger modules)
- **No exceptions**: Return error codes or nullptr
- **No RTTI**: No `dynamic_cast`, use virtual methods or type enums
- **DISALLOW macros**: Prevent unintended copying
- **Singleton pattern**: For global managers (`task_queue_manager`)
- **Weak callbacks**: For preventing use-after-free in async code
- **OBJECT libraries**: Each sub-module compiles as OBJECT, aggregated into `core`

## Testing

Unit tests are co-located as `*_unittest.cc` next to the source files they test. They link against `core` (static) with `TRAA_UNIT_TEST` defined, which:
- Removes `TRAA_API` export decorations
- Enables `TRAA_METRICS_ENABLED` for metrics testing
- Makes some private members accessible (e.g., `task_queue` constructor)
