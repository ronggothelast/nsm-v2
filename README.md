# Nift v2

C++20 static site generator. Nine modular crates, work-stealing build pipeline, SIMD-scanned templates, sandboxed Lua plugins, incremental BLAKE3 cache.

[Website](https://ronggothelast.github.io/nsm-v2/) · [Docs](https://ronggothelast.github.io/nsm-v2/docs.html) · [Install](https://ronggothelast.github.io/nsm-v2/install.html) · [CLI Reference](https://ronggothelast.github.io/nsm-v2/cli.html) · [Architecture](https://ronggothelast.github.io/nsm-v2/architecture.html) · [Changelog](CHANGELOG.md) · [Upstream v1](https://github.com/nifty-site-manager/nsm)

---

**Status:** Phase 7 complete. 272/272 tests passing. CI green on Linux, macOS, Windows.

This is a ground-up rewrite of [Nift v1](https://github.com/nifty-site-manager/nsm). The v1 source is preserved read-only in [`legacy/`](legacy/).

---

## What it does

- Single static binary. No runtime dependencies.
- Context-aware lexer + recursive descent parser. Errors point at lines and columns.
- Work-stealing parallel build executor. Files processed across N cores.
- SIMD template scanner (xsimd, AVX2). Measured 2.4× vs scalar on 1 MiB sparse templates.
- BLAKE3 incremental cache. Changed 1 file? Only that file rebuilds.
- Sandboxed Lua plugins via sol2. Memory cap, instruction budget, no filesystem/shell access.
- Native plugins via stable C ABI (`dlopen` / `LoadLibrary`).
- Built-in dev server with file watching and livereload.
- v1 → v2 migrator: `nift migrate ./old-project`.

## Benchmarks

Measured on commit `d264a19`. All benchmarks checked into repo, re-run in CI.

| Workload | Result |
|---|---|
| SIMD scanner, 1 MiB sparse `@` markers | 0.37 ns/byte (2.4× vs scalar) |
| Cold build, 200 pages | ~95 ms |
| Warm build, 1 file changed | ~6 ms |
| Unit test suite | 272/272 pass, 4.2s wall |

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

Fork, branch, commit with conventional prefixes (`feat:`, `fix:`, `perf:`, `docs:`, `test:`). Run `just test` before opening a PR. `just fmt` for clang-format.

See [CHANGELOG.md](CHANGELOG.md) for current state and [docs/adr/](docs/adr/) for architectural decisions.

## Upstream

- **Original:** [nifty-site-manager/nsm](https://github.com/nifty-site-manager/nsm)
- **Author:** Nicholas Ham (MIT, 2015-present)
- **Website:** https://nift.dev

## License

[MIT](LICENSE) © Nicholas Ham (2015-present) & Nift v2 contributors.
