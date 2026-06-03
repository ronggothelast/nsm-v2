# Nift v2

C++20 static site generator. Thirteen modular crates, work-stealing build pipeline, SIMD-scanned templates, sandboxed Lua plugins, incremental BLAKE3 cache.

[Website](https://ronggothelast.github.io/nsm-v2/) · [Docs](https://ronggothelast.github.io/nsm-v2/docs.html) · [Install](https://ronggothelast.github.io/nsm-v2/install.html) · [CLI Reference](https://ronggothelast.github.io/nsm-v2/cli.html) · [Architecture](https://ronggothelast.github.io/nsm-v2/architecture.html) · [Changelog](CHANGELOG.md) · [Upstream v1](https://github.com/nifty-site-manager/nsm)

---

**Status:** Phase 8 complete. 445/445 tests passing. CI green on Linux, macOS, Windows.

This is a ground-up rewrite of [Nift v1](https://github.com/nifty-site-manager/nsm). The v1 source is preserved read-only in [`legacy/`](legacy/).

---

## What it does

- Single binary. Links only libc, libstdc++, libgcc_s (standard on all Linux distros). Static build available: `cmake --preset release -DNIFT_STATIC_LINK=ON`.
- Context-aware lexer + recursive descent parser. Errors point at lines and columns.
- Work-stealing parallel build executor. Files processed across N cores.
- SIMD template scanner (xsimd, AVX2). Measured 2.4× vs scalar on 1 MiB sparse templates.
- BLAKE3 incremental cache with CBOR binary format. Changed 1 file? Only that file rebuilds.
- Sandboxed Lua plugins via sol2. Memory cap, instruction budget, no filesystem/shell access.
- Native plugins via stable C ABI v2 (`dlopen` / `LoadLibrary`) with structured args.
- Built-in dev server with native FS watch (inotify) and SSE livereload.
- Template inheritance: `@extends`, `@section`, `@yield`, `@parent`.
- Markdown (GFM) with front matter parsing (YAML/JSON) and syntax highlighting (14 languages).
- Tailwind CSS integration: auto-config generation, class scanning, subprocess compilation.
- Asset pipeline: CSS minification (lightningcss), JS bundling (esbuild), BLAKE3 fingerprinting.
- Image optimization: resize, WebP/AVIF conversion, responsive srcset generation, `<picture>` tags.
- v1 → v2 migrator: `nift migrate ./old-project`.

## Benchmarks

Measured on commit `d264a19`. All benchmarks checked into repo, re-run in CI.

| Workload | Result |
|---|---|
| SIMD scanner, 1 MiB sparse `@` markers | 0.37 ns/byte (2.4× vs scalar) |
| Cold build, 200 pages | ~95 ms |
| Warm build, 1 file changed | ~6 ms |
| Unit test suite | 445/445 pass, 18.1s wall |

## Install

Pre-built binaries: [Releases](https://github.com/ronggothelast/nsm-v2/releases)

```bash
# Linux
curl -LO https://github.com/ronggothelast/nsm-v2/releases/latest/download/nift-linux-x86_64.tar.gz
tar xzf nift-linux-x86_64.tar.gz
sudo mv nift /usr/local/bin/
```

Or build from source:

```bash
git clone --recurse-submodules https://github.com/ronggothelast/nsm-v2.git
cd nsm-v2
cmake --preset release
cmake --build --preset release
```

**Requirements:** CMake ≥ 3.25, C++20 compiler (GCC 11+, Clang 14+, MSVC 2022+), Git.

## Quick start

```bash
nift init my-site
cd my-site
nift build     # Built 1 · cached 0 · failed 0 in 0 ms
nift serve     # http://127.0.0.1:8080
```

## Repo layout

```
crates/core/       Path, FS, Str, DateTime, Result types
crates/parser/     Lexer, parser, AST, evaluator
crates/runtime/    Lua bridge (sol2), expression eval
crates/project/    ProjectConfig, BuildCache (BLAKE3 + JSON)
crates/build/      Pipeline, work-stealing executor, mmap, SIMD scanner
crates/server/     HTTP server (cpp-httplib), file watcher
crates/cli/        argparse + subcommands
crates/plugin/     C-ABI dynamic plugin loader
crates/compat/     v1 → v2 migrator
crates/markdown/   GFM parser (cmark-gfm), front matter, code highlighting
crates/tailwind/   Tailwind CSS integration (subprocess wrapper)
crates/assets/     Asset pipeline: CSS minify, JS bundle, BLAKE3 fingerprinting
crates/images/     Image optimization: resize, WebP/AVIF, responsive srcset
apps/nift/         Main binary
legacy/            Nift v1 snapshot (read-only)
docs/              Architecture decision records, recon reports
website/           Documentation site (GitHub Pages)
```

## Migrating from v1

```bash
nift migrate ./old-project
# detected   nsm.config (v1 schema)
# written    nift.json
# preserved  nsm.config.backup
```

Template syntax is backward compatible. The migrator handles config translation automatically.

## Contributing

Fork, branch, commit with conventional prefixes (`feat:`, `fix:`, `perf:`, `docs:`, `test:`). Run `just test` before opening a PR. `just fmt` for clang-format. See [CONTRIBUTING.md](CONTRIBUTING.md) for full details.

Available `just` recipes:

```
just build            configure + build debug
just test             build + run tests
just test-asan        build + test with AddressSanitizer
just test-ubsan       build + test with UBSan
just test-coverage    build + test + coverage data
just fmt              clang-format fix
just lint             clang-tidy
just ci               build + test (local CI)
just qa-fuzz          fuzz parser + lexer (Clang, 60s each)
```

See [CHANGELOG.md](CHANGELOG.md) for current state and [docs/adr/](docs/adr/) for architectural decisions.

## Upstream

- **Original:** [nifty-site-manager/nsm](https://github.com/nifty-site-manager/nsm)
- **Author:** Nicholas Ham (MIT, 2015-present)
- **Website:** https://nift.dev

## License

[MIT](LICENSE) © Nicholas Ham (2015-present) & Nift v2 contributors.
