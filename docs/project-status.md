# traa Project Status

This document captures the current observed status of the repository based on the code layout,
public API, build scripts, and tests in the local checkout.

## Snapshot

- Repository branch: `main`
- Working tree: clean at inspection time
- Primary implementation focus today: desktop screen source enumeration and snapshot support
- Primary unfinished areas: device management, broader media pipeline, mobile-facing API maturity,
  and Linux completeness

## What Looks Production-Oriented

- Cross-platform CMake build scaffolding exists for Windows, macOS, Linux, Android, iOS, and xrOS.
- CI workflows cover major target platforms.
- The public C API is intentionally small and stable-looking.
- The screen-capture subsystem has substantial implementation depth and a real test footprint.

## What Is Still Early

- `engine` is only partially implemented.
- device enumeration APIs are exported but not functionally complete yet.
- Linux support is partial and currently centered on X11.
- mobile targets have packaging/build work, but not equivalent end-user feature completeness.
- documentation had fallen behind the code in a few places.

## Platform Capability Summary

| Platform | Build Scaffold | Screen Source Enumeration | Snapshot | Notes |
| --- | --- | --- | --- | --- |
| Windows | Yes | Yes | Yes | Most complete desktop path set |
| macOS | Yes | Yes | Yes | Includes permission checks and multiple capture paths |
| Linux | Yes | Partial | No | X11 path only, Wayland path incomplete |
| Android | Yes | No clear parity | No clear parity | Packaging exists, public integration still thin |
| iOS | Yes | Not yet comparable | Not yet comparable | Build target exists |
| xrOS | Yes | Not yet comparable | Not yet comparable | Build target exists |

## Near-Term Priorities That Would Improve the Project Most

1. Finish `engine` responsibilities for event handling and device management.
2. Bring README and public docs into sync with actual platform support.
3. Decide whether Linux should remain X11-first or grow a real Wayland path.
4. Replace Android demo-level wrapper code with a real public binding surface.
5. Clarify the roadmap from snapshot/enumeration utilities to full capture/stream workflows.
