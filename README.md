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
    56|nsm-v2/
    57|├── legacy/              # ✅  Nift v1 snapshot (read-only reference)
    58|├── docs/
    59|│   ├── recon/           # ✅  Phase 0 reports
    60|│   ├── adr/             # ✅  Architecture Decision Records
    61|│   └── ...              # ⏳  user docs (mkdocs-material) — Phase 7
    62|├── crates/              # ⏳  modular C++23 crates (core / parser / runtime / ...)
    63|├── apps/nift/           # ⏳  CLI binary entry — Phase 6
    64|├── third_party/         # ⏳  pinned deps via vcpkg manifest — Phase 1
    65|├── tests/               # ⏳  unit + integration + fuzz + bench
    66|├── packaging/           # ⏳  deb / rpm / brew / choco / AUR / nix / docker
    67|├── .github/workflows/   # ⏳  CI matrix — Phase 1
    68|├── CMakeLists.txt       # ⏳  Phase 1
    69|├── CMakePresets.json    # ⏳  Phase 1
    70|├── vcpkg.json           # ⏳  Phase 1
    71|├── Justfile             # ⏳  Phase 1
    72|├── CHANGELOG.md         # ✅
    73|└── README.md            # ✅
    74|```
    75|
    76|---
    77|
    78|## 🏗️ Build from source (post-Phase 1)
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
    90|**Toolchain requirements (Phase 1+):**
    91|- CMake ≥ 3.25
    92|- C++23 compiler: GCC ≥ 13, Clang ≥ 17, MSVC ≥ 19.38 (VS 2022 17.8+)
    93|- Git
    94|- (Optional) `ccache` / `sccache`, `mold` / `lld`
    95|
    96|Build profiles (presets, set up in Phase 1):
    97|
    98|```bash
    99|cmake --preset debug          # debug symbols, no opt
   100|cmake --preset release        # -O3 + LTO
   101|cmake --preset asan           # AddressSanitizer
   102|cmake --preset ubsan          # UBSan
   103|cmake --preset tsan           # ThreadSanitizer
   104|cmake --preset wasm           # Emscripten → nift.wasm
   105|cmake --preset static-musl    # static Linux binary
   106|cmake --preset mingw          # cross-compile Windows
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