# Changelog

All notable changes to **Nift v2** (`nsm-v2`) are documented here.
Format inspired by [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning follows [Semantic Versioning](https://semver.org/).

Conventional commits drive most entries — see [`docs/adr/0000-process.md`](docs/adr/0000-process.md).

---

## [Unreleased]

### Added — Phase 1 (Foundation)
- **CMake build system** with `CMakePresets.json` (debug, release, asan, ubsan, tsan, ci presets).
- **vcpkg manifest** (`vcpkg.json`) — deps: `fmt 12.1`, `spdlog 1.17`, `Catch2 3.15`. Baseline pinned.
- **`crates/core/` library** — modern C++20/23 rewrites of Path, FileSystem, StrFns, Quoted, DateTime, SystemInfo.
- **`nift::Expected<T,E>` polyfill** in `types.hpp` — `std::expected` when available, minimal fallback otherwise.
- **62 unit tests** via Catch2 — all pass, covering types, path, filesystem, string, quoted, datetime, sysinfo.
- **`.clang-format`** (Google-based, 88-col), **`.clang-tidy`** (bugprone/modernize/performance), **`.editorconfig`**.
- **`Justfile`** — task runner with `build`, `test`, `test-asan`, `test-ubsan`, `fmt`, `lint`, `ci` recipes.
- **GitHub Actions CI** (`.github/workflows/ci.yml`) — Linux (gcc 13/14, clang 17/18) × macOS × Windows × sanitizers.
- **ADRs**: 0001 (CMake), 0002 (vcpkg), 0003 (C++20/23 baseline).

### Changed
- `README.md` phase tracker updated (Phase 0 ✅, Phase 1 ✅).

### Tag
- `v2-phase-1-foundation`

---

## [Phase 2 — Parser Rewrite]

### Added — Phase 2 (Parser)
- **`crates/parser/`** — complete template language parser: lexer, parser, AST, evaluator.
- **Context-aware lexer** — only `@` and `$` are special in text mode; `\@`/`\$` escapes supported.
- **Token types**: Text, Directive, VarSimple, VarBracket, VarExpr, LParen, RParen, Comma, LineComment, BlockComment, Eof.
- **Recursive descent parser** — handles `@name{opts}(params)` + block directives (`@if`/`@for`/`@while`).
- **AST nodes**: TextNode, VarNode (3 kinds), DirectiveNode, BlockNode (with elif/else), CommentNode, ProgramNode.
- **Evaluator** — visitor pattern, handles @if/@for/@while/@def/@set/@write/@echo/@input/@content/@exit.
- **Multi-token param extraction** — `$var` inside `(...)` correctly parsed across token boundaries.
- **Block body whitespace trimming** — leading newline after `{` consumed, preserving expected output.
- **62 new parser tests** — total 124 tests, all passing.

### Tag
- `v2-phase-2-parser`

---

## [Phase 3 — Runtime + Project]

### Added — Phase 3 (Runtime)
- **`crates/runtime/`** — Lua sandbox + expression evaluator.
- **`LuaRuntime`** — sol2-based Lua 5.4 wrapper with PIMPL idiom; default sandbox (base/string/table/math only); `enable_unsafe()` opt-in for io/os/package/debug/coroutine.
- **Print capture** — `print(...)` output is captured into `__nift_capture` global and returned from `exec()`.
- **`eval_expr` / `eval_bool`** — recursive-descent expression evaluator: arithmetic (`+ - * / %`), comparison (`== != < > <= >=`), logic (`&& || !`), string concat (`..`), variable lookup, parens, string/number literals.
- **35 runtime tests** — Lua bridge round-trip, sandbox enforcement, expression precedence, variable lookup.

### Added — Phase 3 (Project)
- **`crates/project/`** — ProjectConfig + BuildCache.
- **`ProjectConfig`** — declarative site config (name, dirs, ignored paths) with JSON load/save round-trip via `nlohmann/json`.
- **`hash_content(s)` / `hash_file(p)` / `hash_combined(...)`** — BLAKE3-256 hex digests; `hash_combined` is order-independent (sorts inputs).
- **`BuildCache`** — persistent incremental build state with atomic save (tmp + rename), is_dirty diffing, JSON v1 schema.
- **20 project tests** — config round-trip, BLAKE3 determinism, cache CRUD + persist.

### Added — Tooling
- vcpkg deps: `lua`, `sol2`, `blake3`, `nlohmann-json` (existing baseline).
- ADR 0004 — Lua bridge sol2 decision.
- ADR 0005 — Build cache BLAKE3 + JSON (CBOR deferred to Phase 4).

### Tag
- `v2-phase-3-runtime`

---

## [Phase 4 — Build pipeline]

### Added — Phase 4
- **`crates/build/`** — orchestrates parallel rendering with mmap reads + SIMD scanning.
- **`WorkStealingPool`** — N-worker thread pool, per-worker LIFO local deque + FIFO random-victim steal. Round-robin submit. Idempotent shutdown. Exception-safe (errors surface via futures).
- **`MmapReader`** — `mio::mmap_source` wrapper exposing `string_view` over the mapped region. Heap-read fallback for 0-byte files and FS edge cases.
- **`scan_byte` / `scan_positions`** — `xsimd`-based byte scanner. AVX2 / SSE4.2 / NEON dispatch at compile time. Sorted match positions. Scalar tail handles non-aligned ends.
- **`Pipeline`** — high-level orchestrator: file discovery → BLAKE3 hash → cache check → build → write. Returns `BuildReport` (built / cached / failed counts, elapsed time, per-file results).
- **34 build tests** — executor parallelism, mmap correctness, SIMD scan boundaries, pipeline cache hits.
- **`bench_simd_scanner`** — micro-benchmark proves SIMD speedup. **Measured: 2.4× faster scalar** on this VM (1 MB buffer, sparse markers).

### Added — Tooling
- vcpkg deps: `mio`, `xsimd`.
- ADR 0006 — Work-stealing pool decision.
- ADR 0007 — mmap + xsimd scanning.
- `tests/bench/` opt-in benchmark target gated by `NIFT_BUILD_BENCH=ON`.

### Changed
- Suppressed `[[nodiscard]] create_directories` warnings in `BuildCache` and `Pipeline` constructors with explicit `(void)` casts (the directory already-exists case is intentional).

### Tag
- `v2-phase-4-build`

---

## [Phase 5 — Dev server + asset pipeline]

### Added — Phase 5
- **`crates/server/`** — dev server, file watcher, asset minify.
- **`HttpServer`** — `cpp-httplib`-based HTTP/1.1 server. Catch-all static file resolver with MIME map (html/css/js/json/png/jpg/svg/ico/webp/woff2/txt). Auto-injects livereload script before `</body>`. Ephemeral port support (port=0).
- **Live reload** — poll-based generation token at `/__nift/livereload`. Client JS at `/__nift/livereload.js` polls every 500 ms and reloads on change. No WebSockets — works through any proxy.
- **`FileWatcher`** — recursive polling watcher. Detects Added / Modified / Removed. Configurable poll interval + ignored substrings (defaults: `.git`, `node_modules`, `build`, `.nift`).
- **`assets::detect_image_format`** — magic-byte sniff for PNG, JPEG, WEBP, GIF, SVG, AVIF.
- **`assets::minify_css`** — comment + whitespace strip, structural-aware spacing, drops trailing `;` before `}`.
- **`assets::minify_js`** — line + block comment strip, preserves string/template literals (handles escaped quotes).
- **34 server tests** — HTTP serve + 404 + livereload + CSS MIME, watcher add/modify/remove + ignore, asset format detection + minification.

### Added — Tooling
- vcpkg dep: `cpp-httplib`.
- ADR 0008 — Dev server: cpp-httplib + polling watcher.

### Tag
- `v2-phase-5-server`

---

## [Phase 6 — CLI + plugin + migrator]

### Added — Phase 6
- **`crates/cli/`** — hand-rolled argparse (`--flag=val`, `--flag val`, bundled bool shorts, `--`) + subcommand dispatcher.
- **`apps/nift/`** — main `nift` CLI binary. Subcommands: `init`, `build`, `serve`, `migrate`, `clean`, `version`, `help`.
- **`crates/plugin/`** — `dlopen`/`LoadLibrary`-based dynamic plugin loader with stable C ABI (`NIFT_PLUGIN_ABI_VERSION = 1`). `PluginRegistry` maps directive name → plugin, validates ABI version on load.
- **`crates/compat/`** — Nift v1 (`nsm`) → v2 migrator. Reads `siteInfo/nsm.config` (or `.nsm/nsm.config`, `nsm.config`), translates known keys (`siteName`, `contentDir`, `siteDir`, `templateDir`, `defaultTemplate`, `buildDir`) to v2 `nift.json`. Records unknown keys in the report. Migrates `tracked.json` → `.nift/tracked.json`. Non-destructive: never deletes v1 files.
- End-to-end CLI flow verified: `nift init /tmp/site && nift build /tmp/site` produces `public/index.html`.

### Added — Tests
- 24 new tests across argparse, migrator, plugin loader.
- Total: 271/271 passing.

### Added — ADR
- ADR 0009 — CLI design + plugin C ABI + v1 migrator.

### Tag
- `v2-phase-6-cli`

---

## [Phase 7 — Docs + packaging + release]

### Added — Phase 7 (Docs)
- **`docs/quickstart.md`** — 60-second walkthrough: install, init, build, serve, migrate.
- **`docs/architecture.md`** — full crate diagram, build pipeline, concurrency model, design rationale, deferred items.
- **`docs/plugin-author.md`** — C ABI contract, minimal example, build instructions, versioning, future extensions.
- **`docs/migration.md`** — v1 → v2 mapping table, directive changes, perf deltas, rollback path.
- **`docs/README.md`** — docs landing page with cross-links.

### Added — Phase 7 (Packaging)
- **`Dockerfile`** — multi-stage Ubuntu 22.04 build, distroless-style runtime.
- **`packaging/homebrew/nift.rb`** — Homebrew formula (needs tap).
- **`packaging/aur/PKGBUILD`** — Arch AUR package recipe.
- **`packaging/debian/control`** — Debian package stub (full DPKG pipeline deferred).
- **`packaging/README.md`** — packager guide + verification instructions.

### Added — Phase 7 (CI)
- **`.github/workflows/ci.yml`** — full matrix:
  - `build-test`: Linux (gcc-11, clang-14), macOS (clang), Windows (MSVC) × Debug
  - `lint`: clang-format-14 dry-run gating
  - `sanitizers`: ASan + UBSan on full test suite
  - `release-binary`: tagged builds publish tarballs as artifacts
- vcpkg binary cache keyed on manifest + baseline.

### Added — Phase 7 (ADR)
- ADR 0010 — Packaging + CI release matrix.

### Tag
- `v2-phase-7-release`

---

### Added — Phase 0 (Recon)
- `docs/recon/` reports: `toolchain.md`, `loc.md`, `deps.md`, `hotpaths.md`, `build-system.md`, `risks.md`.
- `docs/adr/0000-process.md` — how Architecture Decision Records are written and reviewed.
- `CHANGELOG.md` (this file).
- Replaced bootstrap `README.md` with the official template (status banner + phase tracker).

### Findings
- **54** core Nift source files (~33 K LOC, 845 KB).
- **261** vendored files (5.16 MB) across `rapidjson/`, `LuaJIT/`, `Lua-5.3/`, `exprtk/`, `hashtk/` — to be replaced with vcpkg-managed equivalents in Phase 1.
- **`Parser.cpp`** = 446 KB / 16 949 LOC / single function `read_and_process_fn` spans **12 562 lines**.
- **Zero cyclic includes** at the core layer — clean DAG, refactor-friendly.
- **No CMake / no CI / no tests** in v1 → built from scratch in Phase 1.
- Toolchain currently below v2 baseline (`cmake 3.22`, `gcc 11`, `clang 14`); upgrade plan documented.

### Tag
- `v2-phase-0-recon` (planned at end of Phase 0 commit).

---

## Tag history

(empty — repo is new)
