<div align="center">

# 🐰⚡🦡 Nift v2

### *World's fastest C++ static site generator — reborn for 2026 and beyond*

[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/23)
[![Status](https://img.shields.io/badge/status-Phase%206%20CLI-green)](CHANGELOG.md)
[![Upstream](https://img.shields.io/badge/Upstream-nifty--site--manager%2Fnsm-blue)](https://github.com/nifty-site-manager/nsm)

[**Recon report**](docs/recon/README.md) · [**Changelog**](CHANGELOG.md) · [**ADRs**](docs/adr/) · [**Upstream v1**](https://github.com/nifty-site-manager/nsm)

</div>

---

> ✅ **Status: Phase 6 CLI + plugin + migrator complete.** Phase 7 (Docs + packaging + signed release) next.
> This is the working repository for the **Nift v2** rewrite. The v1 source is preserved read-only in [`legacy/`](legacy/) as the migration reference.

---

## ⚡ What's Nift?

**Nift** is a **C++23 static site generator** built around one obsession: **speed**.

- 🚀 Target: **2-5× faster** than Hugo on equivalent workloads
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
| 1 | Foundation — CMake presets, `core/` crate, CI matrix | ✅ **complete** |
| 2 | Parser rewrite — split 446 KB monolith | ✅ **complete** |
| 3 | Runtime + Project — Lua bridge, incremental cache | ✅ **complete** |
| 4 | Build pipeline — work-stealing, SIMD, mmap | ✅ **complete** |
| 5 | Dev server + asset pipeline | ✅ **complete** |
| 6 | CLI + plugin system + migrator | ✅ **complete** |
| 7 | Docs + packaging + signed release | ⏳ pending |

Phase tags published as `v2-phase-N-<name>` on `main`.

---

## 📂 Repo layout

````text
nsm-v2/
├── legacy/              # ✅  Nift v1 snapshot (read-only reference)
├── docs/
│   ├── recon/           # ✅  Phase 0 reports
│   ├── adr/             # ✅  Architecture Decision Records
│   └── ...              # ⏳  user docs (mkdocs-material) — Phase 7
├── crates/core/         # ✅  Phase 1 — Path, FS, Str, DateTime, SysInfo
├── crates/parser/       # ✅  Phase 2 — Lexer, Parser, AST, Evaluator
├── crates/runtime/      # ✅  Phase 3 — Lua bridge (sol2), expression evaluator
├── crates/project/      # ✅  Phase 3 — ProjectConfig, BuildCache (BLAKE3 + JSON)
├── crates/build/        # ✅  Phase 4 — work-stealing pool, mmap, SIMD scanner, pipeline
├── crates/server/       # ✅  Phase 5 — HTTP dev server, file watcher, asset minify
├── crates/cli/          # ✅  Phase 6 — argparse + subcommands
├── crates/plugin/       # ✅  Phase 6 — C ABI dynamic plugin loader
├── crates/compat/       # ✅  Phase 6 — v1 nsm.config → v2 nift.json migrator
├── apps/nift/           # ✅  Phase 6 — main `nift` CLI binary
├── third_party/vcpkg    # ✅  vcpkg submodule (fmt, spdlog, Catch2, lua, sol2, blake3, json, mio, xsimd, cpp-httplib)
├── tests/               # ✅  271 unit tests (Catch2) + bench — Phase 1+2+3+4+5+6
├── packaging/           # ⏳  deb / rpm / brew / choco / AUR / nix / docker
├── .github/workflows/   # ✅  CI matrix (gcc/clang/macOS/Windows/sanitizers)
├── CMakeLists.txt       # ✅  Phase 1
├── CMakePresets.json    # ✅  Phase 1 (debug/release/asan/ubsan/tsan/ci)
├── vcpkg.json           # ✅  Phase 1 (fmt, spdlog, Catch2)
├── Justfile             # ✅  Phase 1
├── LICENSE              # ✅  MIT (upstream + v2 contributors)
├── CHANGELOG.md         # ✅
└── README.md            # ✅
````

---

## 🏗️ Build from source

```bash
git clone https://github.com/ronggothelast/nsm-v2
cd nsm-v2
git submodule update --init --depth 1

cmake --preset release
cmake --build --preset release -j
ctest --preset release
```

**Toolchain requirements:**

- CMake ≥ 3.22 (4.x via `pip install cmake` recommended)
- C++20 compiler (C++23 auto-detected when available)
- GCC ≥ 11, Clang ≥ 14, MSVC ≥ 19.34 (VS 2022)
- Git
- (Optional) `ccache` / `sccache`, `mold` / `lld`

**Build presets:**

```bash
cmake --preset debug          # debug symbols, no opt
cmake --preset release        # -O3, tests on
cmake --preset asan           # AddressSanitizer
cmake --preset ubsan          # UndefinedBehaviorSanitizer
cmake --preset tsan           # ThreadSanitizer
cmake --preset ci             # release + warnings-as-errors (for CI)
```

**Task runner (just):**

```bash
just build                    # configure + build debug
just test                     # build + run tests
just test-asan                # build + test with ASan
just fmt                      # clang-format fix
just lint                     # clang-tidy
just ci                       # build + test (local CI)
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

This is an active rewrite. Read [`CHANGELOG.md`](CHANGELOG.md) for current state and [`docs/recon/README.md`](docs/recon/README.md) for the architectural baseline.

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
