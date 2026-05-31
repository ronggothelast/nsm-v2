     1|<div align="center">
     2|
     3|# 🐰⚡🦡 Nift v2
     4|
     5|### *World's fastest C++ static site generator — reborn for 2026 and beyond*
     6|
     7|[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
     8|[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/23)
     9|[![Status](https://img.shields.io/badge/status-Phase%201%20Foundation-green)](docs/recon/README.md)
    10|[![Upstream](https://img.shields.io/badge/Upstream-nifty--site--manager%2Fnsm-blue)](https://github.com/nifty-site-manager/nsm)
    11|
    12|[**Recon report**](docs/recon/README.md) · [**Changelog**](CHANGELOG.md) · [**ADRs**](docs/adr/) · [**Upstream v1**](https://github.com/nifty-site-manager/nsm)
    13|
    14|</div>
    15|
    16|---
    17|
    18|> ✅ **Status: Phase 1 Foundation complete.** Phase 2 (Parser rewrite) next.
    19|> This is the working repository for the **Nift v2** rewrite. The v1 source is preserved read-only in [`legacy/`](legacy/) as the migration reference.
    20|
    21|---
    22|
    23|## ⚡ What's Nift?
    24|
    25|**Nift** is a **C++23 static site generator** built around one obsession: **speed**.
    26|
    27|- 🚀 Target: **2-5× faster** than Hugo on equivalent workloads (see [`BENCHMARKS.md`](BENCHMARKS.md) — populated Phase 4+)
    28|- 🧠 **Smart incremental builds** — only rebuild what changed, persisted BLAKE3 cache
    29|- 🎨 **Built-in asset pipeline** — Tailwind v4, esbuild, libvips image optimization
    30|- 🔌 **Pluggable** — stable C ABI, Lua scripts, sandboxed WASM modules
    31|- 🌐 **Universal** — Linux, macOS, Windows, BSD, WASM (runs in your browser)
    32|- 📦 **Zero runtime** — single static binary, no Node / Python / Ruby needed
    33|
    34|---
    35|
    36|## 📍 Where we are
    37|
    38|| Phase | Name | Status |
    39||---|---|---|
    40|| 0 | Recon — scan legacy, dep graph, hot paths, toolchain | ✅ **complete** |
    41|| 1 | Foundation — CMake presets, `core/` crate, CI matrix | ✅ **complete** |
    42|| 2 | Parser rewrite — split 446 KB monolith | ⏳ pending |
    43|| 3 | Runtime + Project — Lua bridge, incremental cache | ⏳ pending |
    44|| 4 | Build pipeline — work-stealing, SIMD, mmap | ⏳ pending |
    45|| 5 | Dev server + asset pipeline | ⏳ pending |
    46|| 6 | CLI + plugin system + migrator | ⏳ pending |
    47|| 7 | Docs + packaging + signed release | ⏳ pending |
    48|
    49|Phase tags published as `v2-phase-N-<name>` on `main`.
    50|
    51|---
    52|
    53|## 📂 Repo layout (target — populated phase by phase)
    54|
    55|```
nsm-v2/
├── legacy/              # ✅  Nift v1 snapshot (read-only reference)
├── docs/
│   ├── recon/           # ✅  Phase 0 reports
│   ├── adr/             # ✅  Architecture Decision Records
│   └── ...              # ⏳  user docs (mkdocs-material) — Phase 7
├── crates/core/         # ✅  Phase 1 — Path, FS, Str, DateTime, SysInfo
├── apps/nift/           # ⏳  CLI binary entry — Phase 6
├── third_party/vcpkg    # ✅  vcpkg submodule (fmt, spdlog, Catch2)
├── tests/               # ✅  62 unit tests (Catch2) — Phase 1
├── packaging/           # ⏳  deb / rpm / brew / choco / AUR / nix / docker
├── .github/workflows/   # ✅  CI matrix (gcc/clang/macOS/Windows/sanitizers)
├── CMakeLists.txt       # ✅  Phase 1
├── CMakePresets.json    # ✅  Phase 1 (debug/release/asan/ubsan/tsan/ci)
├── vcpkg.json           # ✅  Phase 1 (fmt, spdlog, Catch2)
├── Justfile             # ✅  Phase 1
├── LICENSE              # ✅  MIT (upstream + v2 contributors)
├── CHANGELOG.md         # ✅
└── README.md            # ✅
    74|```
    75|
    76|---
    77|
    78|## 🏗️ Build from source
    79|
    80|```bash
    81|git clone https://github.com/ronggothelast/nsm-v2
    82|cd nsm-v2
    83|
    84|cmake --preset release
    85|cmake --build --preset release -j
    86|sudo cmake --install build/release
    87|ctest --preset release
    88|```
    89|
**Toolchain requirements:**
- CMake ≥ 3.22 (4.x via pip recommended)
- C++20 compiler (C++23 auto-detected when available)
- GCC ≥ 11, Clang ≥ 14, MSVC ≥ 19.34 (VS 2022)
    93|- Git
    94|- (Optional) `ccache` / `sccache`, `mold` / `lld`
    95|
    96|Build profiles (presets, set up in Phase 1):
    97|
    98|```bash
cmake --preset debug          # debug symbols, no opt
cmake --preset release        # -O3, -Werror, tests on
cmake --preset asan           # AddressSanitizer
cmake --preset ubsan          # UndefinedBehaviorSanitizer
cmake --preset tsan           # ThreadSanitizer
cmake --preset ci             # release + Werror (for CI)
   107|```
   108|
   109|---
   110|
   111|## 🧬 Migrating from Nift v1 (Phase 6+)
   112|
   113|```bash
   114|cd my-old-nift-project
   115|nift migrate              # automatic, idempotent, dry-run by default
   116|nift migrate --apply      # commits the conversion
   117|```
   118|
   119|---
   120|
   121|## 🤝 Contributing
   122|
   123|This is an active rewrite. Read [`CHANGELOG.md`](CHANGELOG.md) for current state and `docs/recon/README.md` for the architectural baseline.
   124|
   125|Conventional commits enforced: `feat:`, `fix:`, `perf:`, `refactor:`, `docs:`, `test:`, `chore:`, `ci:`, `build:`.
   126|
   127|---
   128|
   129|## 🔗 Upstream
   130|
   131|- **Original:** [`nifty-site-manager/nsm`](https://github.com/nifty-site-manager/nsm)
   132|- **Author:** Nicholas Ham (MIT, 2015-present)
   133|- **Website:** https://nift.dev
   134|
   135|This fork stays MIT and credits the upstream author in every release.
   136|
   137|---
   138|
   139|## 📜 License
   140|
   141|[MIT](LICENSE) © Nicholas Ham (2015-present) & Nift v2 contributors.
   142|
   143|---
   144|
   145|<div align="center">
   146|
   147|Made with C++ and ❤️.
   148|
   149|</div>
   150|