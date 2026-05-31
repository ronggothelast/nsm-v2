# ADR-0003 — C++ standard: C++20 baseline with C++23 feature detection

**Status:** Accepted
**Date:** Phase 1 (Foundation)
**Deciders:** lead architect agent

---

## Context

The spec mandates C++23. However, compiler availability varies:
- Ubuntu 22.04 ships gcc 11 / clang 14 (C++20 complete, C++23 partial).
- Ubuntu 24.04 ships gcc 14 / clang 18 (C++23 nearly complete).
- macOS Xcode 15 Apple Clang supports most C++20, partial C++23.
- MSVC 19.38+ supports most C++23.

Requiring C++23 exclusively would lock out contributors on older systems and break CI on some platforms.

## Decision

**C++20 as the build floor**, with **C++23 features opt-in via `__cpp_*` feature-test macros and `__has_include`**.

Concrete approach:
- `CMAKE_CXX_STANDARD 20` is the default.
- CMake detects `-std=c++23` support; if available, upgrades to 23.
- `std::expected<T,E>` → polyfill in `types.hpp` (minimal `Expected` class) when `<expected>` is absent.
- `std::format` / `std::print` → `fmt::format` / `fmt::print` always (via vcpkg `fmt` package). When C++23 `<print>` lands everywhere, we can alias.
- `std::flat_map`, `std::mdspan` → use when available, skip otherwise.

## Alternatives considered

- **C++23 hard requirement** — rejected. Would exclude gcc < 13, clang < 17, and many CI runners.
- **C++17 baseline** — too conservative. Loses `std::span`, concepts, ranges, coroutines, `std::filesystem` guarantees.
- **C++20 only, no C++23** — viable but leaves performance on the table (`std::expected` eliminates exception overhead in hot paths).

## Consequences

- **Positive:** builds on any compiler from the last 3 years.
- **Positive:** C++23 features used where they matter most (error handling, hot paths) via polyfill.
- **Negative:** polyfill for `Expected` adds ~80 lines of code to maintain.
- **Negative:** can't use C++23 modules (`import std;`) as primary — header fallback everywhere.

## References

- `docs/recon/toolchain.md` — current compiler versions.
- Feature test macros: https://en.cppreference.com/w/cpp/feature_test
