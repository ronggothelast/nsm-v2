# Changelog

All notable changes to **Nift v2** (`nsm-v2`) are documented here.
Format inspired by [Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versioning follows [Semantic Versioning](https://semver.org/).

Conventional commits drive most entries — see [`docs/adr/0000-process.md`](docs/adr/0000-process.md).

---

## [Unreleased]

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
