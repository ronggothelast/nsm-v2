# Phase 0 — Recon Reports

Reconnaissance of Nift v1 (`legacy/` snapshot) performed before Phase 1 Foundation.
All findings are based on the source tree at `legacy/` as of repo init.

## Reports in this folder

| File | What it covers |
|---|---|
| [`toolchain.md`](toolchain.md) | Build environment, compilers, missing tooling, install plan |
| [`loc.md`](loc.md) | Lines of code per file, separation of core vs vendored deps |
| [`deps.md`](deps.md) | Include dependency graph, cycle detection, vendored libs inventory |
| [`hotpaths.md`](hotpaths.md) | Functions identified as hot paths from the 446 KB `Parser.cpp` monolith |
| [`build-system.md`](build-system.md) | Legacy `Makefile` analysis — flags, conditionals, platform branches |
| [`risks.md`](risks.md) | Risks, anti-patterns, migration blockers identified during recon |

## TL;DR

- **54 core Nift source files** (~33 K LOC, 845 KB) — modular at the file level **but** dominated by `Parser.cpp` (446 KB / 16.9 K LOC, 51 % of all core code).
- **261 vendored files** (5.2 MB) across `rapidjson/`, `LuaJIT/`, `Lua-5.3/`, `exprtk/`, `hashtk/` — all will be replaced by vcpkg/Conan-managed equivalents in Phase 1.
- **Zero cyclic includes** at the core level — clean DAG, refactor-friendly.
- **No CMake**, no presets, no CI, no tests. Single hand-rolled `Makefile` (368 lines, ~50 platform conditionals).
- **C++11** baseline — will jump to **C++23** with `std::expected`, `std::print`, modules where available.
- Largest functions in `Parser.cpp` reach 1000+ lines — split mandatory in Phase 2.

Next: Phase 1 — Foundation (CMake skeleton, `core/` crate, CI matrix).
