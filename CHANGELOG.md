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
