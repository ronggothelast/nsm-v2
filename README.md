<div align="center">

# 🐰⚡🦡 Nift v2

### *World's fastest C++ static site generator — reborn for 2026 and beyond*

[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/23)
[![Status](https://img.shields.io/badge/status-Phase%200%20Recon-yellow)](docs/recon/README.md)
[![Upstream](https://img.shields.io/badge/Upstream-nifty--site--manager%2Fnsm-blue)](https://github.com/nifty-site-manager/nsm)

[**Recon report**](docs/recon/README.md) · [**Changelog**](CHANGELOG.md) · [**ADRs**](docs/adr/) · [**Upstream v1**](https://github.com/nifty-site-manager/nsm)

</div>

---

> ⚠ **Status: 🟡 Phase 0 Recon complete.** Phase 1 (Foundation) not yet started.
> This is the working repository for the **Nift v2** rewrite. The v1 source is preserved read-only in [`legacy/`](legacy/) as the migration reference.

---

## ⚡ What's Nift?

**Nift** is a **C++23 static site generator** built around one obsession: **speed**.

- 🚀 Target: **2-5× faster** than Hugo on equivalent workloads (see [`BENCHMARKS.md`](BENCHMARKS.md) — populated Phase 4+)
- 🧠 **Smart incremental builds** — only rebuild what changed, persisted BLAKE3 cache
- 🎨 **Built-in asset pipeline** — Tailwind v4, esbuild, libvips image optimization
- 🔌 **Pluggable** — stable C ABI, Lua scripts, sandboxed WASM modules
- 🌐 **Universal** — Linux, macOS, Windows, BSD, WASM (runs in your browser)
- 📦 **Zero runtime** — single static binary, no Node / Python / Ruby needed

---

## 📍 Where we are

| Phase | Name | Status |
|---|---|---|
| 0 | Recon — scan legacy, dep graph, hot paths, toolchain | ✅ **complete** |
| 1 | Foundation — CMake presets, `core/` crate, CI matrix | ⏳ pending |
| 2 | Parser rewrite — split 446 KB monolith | ⏳ pending |
| 3 | Runtime + Project — Lua bridge, incremental cache | ⏳ pending |
| 4 | Build pipeline — work-stealing, SIMD, mmap | ⏳ pending |
| 5 | Dev server + asset pipeline | ⏳ pending |
| 6 | CLI + plugin system + migrator | ⏳ pending |
| 7 | Docs + packaging + signed release | ⏳ pending |

Phase tags published as `v2-phase-N-<name>` on `main`.

---

## 📂 Repo layout (target — populated phase by phase)

```
nsm-v2/
├── legacy/              # ✅  Nift v1 snapshot (read-only reference)
├── docs/
│   ├── recon/           # ✅  Phase 0 reports
│   ├── adr/             # ✅  Architecture Decision Records
│   └── ...              # ⏳  user docs (mkdocs-material) — Phase 7
├── crates/              # ⏳  modular C++23 crates (core / parser / runtime / ...)
├── apps/nift/           # ⏳  CLI binary entry — Phase 6
├── third_party/         # ⏳  pinned deps via vcpkg manifest — Phase 1
├── tests/               # ⏳  unit + integration + fuzz + bench
├── packaging/           # ⏳  deb / rpm / brew / choco / AUR / nix / docker
├── .github/workflows/   # ⏳  CI matrix — Phase 1
├── CMakeLists.txt       # ⏳  Phase 1
├── CMakePresets.json    # ⏳  Phase 1
├── vcpkg.json           # ⏳  Phase 1
├── Justfile             # ⏳  Phase 1
├── CHANGELOG.md         # ✅
└── README.md            # ✅
```

---

## 🏗️ Build from source (post-Phase 1)

```bash
git clone https://github.com/ronggothelast/nsm-v2
cd nsm-v2

cmake --preset release
cmake --build --preset release -j
sudo cmake --install build/release
ctest --preset release
```

**Toolchain requirements (Phase 1+):**
- CMake ≥ 3.25
- C++23 compiler: GCC ≥ 13, Clang ≥ 17, MSVC ≥ 19.38 (VS 2022 17.8+)
- Git
- (Optional) `ccache` / `sccache`, `mold` / `lld`

Build profiles (presets, set up in Phase 1):

```bash
cmake --preset debug          # debug symbols, no opt
cmake --preset release        # -O3 + LTO
cmake --preset asan           # AddressSanitizer
cmake --preset ubsan          # UBSan
cmake --preset tsan           # ThreadSanitizer
cmake --preset wasm           # Emscripten → nift.wasm
cmake --preset static-musl    # static Linux binary
cmake --preset mingw          # cross-compile Windows
```

---

## 🧬 Migrating from Nift v1 (Phase 6+)

```bash
cd my-old-nift-project
nift migrate              # automatic, idempotent, dry-run by default
nift migrate --apply      # commits the conversion
```

---

## 🤝 Contributing

This is an active rewrite. Read [`CHANGELOG.md`](CHANGELOG.md) for current state and `docs/recon/README.md` for the architectural baseline.

Conventional commits enforced: `feat:`, `fix:`, `perf:`, `refactor:`, `docs:`, `test:`, `chore:`, `ci:`, `build:`.

---

## 🔗 Upstream

- **Original:** [`nifty-site-manager/nsm`](https://github.com/nifty-site-manager/nsm)
- **Author:** Nicholas Ham (MIT, 2015-present)
- **Website:** https://nift.dev

This fork stays MIT and credits the upstream author in every release.

---

## 📜 License

[MIT](LICENSE) © Nicholas Ham (2015-present) & Nift v2 contributors.

---

<div align="center">

Made with C++ and ❤️.

</div>
