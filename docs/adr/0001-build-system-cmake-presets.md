# ADR-0001 — Build system: CMake with Presets

**Status:** Accepted
**Date:** Phase 1 (Foundation)
**Deciders:** lead architect agent

---

## Context

Nift v1 uses a hand-rolled `Makefile` (368 lines, ~50 platform conditionals). It works but has no preset system, no IDE integration, no dependency management, and platform branches are brittle shell conditionals.

v2 needs: cross-platform builds (Linux/macOS/Windows/BSD/WASM), sanitizer presets, vcpkg integration, Ninja generator support, and IDE discoverability.

## Decision

**CMake 3.22+** with **CMakePresets.json** (version 6) as the primary build system. **Ninja** as the default generator. **`just`** as the task runner (replaces `make` for human ergonomics).

## Alternatives considered

- **Meson** — excellent DX, native Ninja, but weaker vcpkg integration and less CI ecosystem support. Would require rewriting all vcpkg toolchain logic.
- **Bazel** — overkill for a single-binary project. Massive learning curve, poor adoption in C++ SSG space.
- **xmake** — promising but immature ecosystem, small community.
- **Plain Makefile** — what v1 uses. Rejected for all the reasons listed in Context.

## Consequences

- **Positive:** IDE support (CLion, VS Code CMake Tools, Visual Studio) works out of the box via presets.
- **Positive:** vcpkg manifest mode integrates natively via `CMAKE_TOOLCHAIN_FILE`.
- **Positive:** Sanitizer/release/debug builds are one `--preset` flag.
- **Negative:** CMake syntax is awkward; mitigated by keeping `CMakeLists.txt` minimal per crate.
- **Negative:** CMake 3.22 is the floor (Ubuntu 22.04 default); some preset features need 3.25+ but we work around them.

## References

- `docs/recon/build-system.md` — legacy Makefile analysis.
- CMakePresets.json spec: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
