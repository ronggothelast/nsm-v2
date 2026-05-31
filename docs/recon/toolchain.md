# Toolchain Recon

Environment: Ubuntu 22.04 (jammy), x86_64, container.

## What's installed (Phase 0 verification)

| Tool | Required (v2) | Currently | Status |
|---|---|---|---|
| `cmake` | ≥ 3.25 | **3.22.1** | ⚠ upgrade in Phase 1 (Kitware APT repo) |
| `gcc` | ≥ 13 (C++23) | **11.4.0** | ⚠ upgrade in Phase 1 (`ppa:ubuntu-toolchain-r/test`) |
| `g++` | ≥ 13 | (paired with gcc 11) | ⚠ as above |
| `clang` | ≥ 17 | **14.0.0** | ⚠ install via apt.llvm.org script |
| `cloc` | for recon | **1.90** | ✅ |
| `git` | any modern | present | ✅ |
| `gh` CLI | for PRs | authenticated as `ronggothelast` | ✅ |
| `just` | task runner | **not installed** | install in Phase 1 (`cargo install just` or apt) |
| `vcpkg` | dep manager | not installed | bootstrap in Phase 1 (`git submodule` or system) |
| `ninja` | CMake generator | not verified | install with cmake upgrade |
| `ccache` / `sccache` | build cache | not installed | install Phase 1 |
| `clang-format` | enforced format | (bundled with clang) | upgrade alongside clang |
| `clang-tidy` | static analysis | (bundled with clang) | upgrade alongside clang |
| `scc` | LOC scan | not installed | optional — `cloc` already covers it |
| `valgrind` | memcheck | not verified | install pre-Phase 4 |

## Network situation

`add-apt-repository ppa:ubuntu-toolchain-r/test` **failed** during recon — DNS error reaching `api.launchpad.net`. Worked around by using default repos (gcc 11 / clang 14 / cmake 3.22 — adequate for Phase 0 recon, not for Phase 1+ build).

**Phase 1 plan:**
1. Retry PPA (transient DNS) — or fall back to Kitware APT (`apt.kitware.com`) for CMake and apt.llvm.org bash script for clang-17.
2. If both fail, document and downgrade to `-std=c++20` baseline; flag C++23 features individually behind `#ifdef __cpp_*` guards.

## Compiler matrix v2 targets (Phase 1 CI)

```yaml
linux:
  - ubuntu-22.04: { gcc: 13, clang: 17 }
  - ubuntu-24.04: { gcc: 14, clang: 18 }
macos:
  - macos-14: { xcode: 15 }  # Apple Clang
windows:
  - windows-2022: { msvc: 19.40, mingw: gcc 14 }
freebsd:
  - cross-build via cirrus-ci or qemu (deferred to Phase 7)
wasm:
  - emscripten 3.1.65 (deferred to Phase 5)
```

## Verification commands

```bash
cmake --version          # need 3.25+
g++-13 --version         # need 13+
clang++-17 --version     # need 17+
ninja --version
just --version
ccache --version
vcpkg version
```

All must pass before Phase 1 PR tag. Failure on any → block phase advance and reinstall.
