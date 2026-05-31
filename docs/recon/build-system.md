# Legacy Build System

Source: `legacy/Makefile` (368 lines).

## What it does

- Single hand-written `Makefile`, no CMake / Meson / Bazel.
- Compiles 26 `.cpp` files plus vendored `hashtk/HashTk.cpp` directly into `nsm` binary.
- `CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic -O3 -Dexprtk_disable_caseinsensitivity`
- `LINK = -pthread`
- **No `-Werror`** in default build (commented out in source).
- No `-Wpedantic` errors caught — silent warnings.
- No sanitizer presets, no debug profile, no LTO toggle, no `-march=native` profile.

## Platform branches (~50 conditionals)

`detected_OS` is computed via `uname`. Branches for:
- **Darwin** (macOS): `-pagezero_size 10000 -image_base 10000000 -Qunused-arguments`
- **Windows** (MinGW): `-s -Wa,-mbig-obj -Wno-cast-function-type ...`
- **FreeBSD**: forces `CXX = clang`, `-lstdc++`
- **Linux/other \*nix**: `-s` (strip).

## Feature flags (preprocessor)

| Flag | Purpose |
|---|---|
| `__NO_CLEAR_LINES__` | Disable terminal cursor clearing (CI / Vercel) |
| `__NO_COLOUR__` | Disable ANSI colors |
| `__NO_PROGRESS__` | Disable progress bars |
| `__BUNDLED__` | Use vendored Lua / LuaJIT instead of system |
| `__LUA_VERSION_5_3__` / `__LUAJIT_VERSION_2_1__` | Pick Lua flavor |
| `VERCEL=1` | Bundle of all three for Vercel deploy |

These all become **CMake options** in v2:
```cmake
option(NIFT_NO_COLOR "Disable ANSI color output" OFF)
option(NIFT_NO_PROGRESS "Disable progress bars" OFF)
option(NIFT_NO_CLEAR_LINES "Disable terminal cursor manipulation" OFF)
set(NIFT_LUA_BACKEND "system" CACHE STRING "lua | luajit | bundled")
```

## Lua bundling logic

`BUNDLED` (default ON in v1) drives a recursive `make` into either `Lua-5.3/src/` (`make all`) or `LuaJIT/src/` (`make amalg`), then statically links `liblua.a` / `libluajit.a`. Brittle on Windows; produces 8-10 MB static binary.

v2 plan: vcpkg-managed Lua / LuaJIT, statically linked when `NIFT_STATIC=ON`. Drop the bundled tree entirely after `compat` test passes.

## What v2 inherits from this Makefile

- Default optimization: `-O3` (keep — Release preset).
- `exprtk` case-insensitivity disable (keep — performance + behavior).
- Pthread link (keep — threading model only expands).
- Platform list (keep — same five targets + WASM via Emscripten).

## What v2 throws away

- All shell conditionals (replaced by CMake `if(APPLE)` / `if(WIN32)` / etc.).
- Recursive Lua sub-make (replaced by FetchContent / vcpkg).
- Manual object list (CMake `file(GLOB_RECURSE ...)` + explicit per-crate `target_sources`).
- VERCEL=1 monolithic flag (replaced by `--preset vercel-ci` in CMakePresets.json).
