# ADR-0002 — Dependency manager: vcpkg manifest mode

**Status:** Accepted
**Date:** Phase 1 (Foundation)
**Deciders:** lead architect agent

---

## Context

Nift v1 vendors all dependencies directly (rapidjson, LuaJIT, Lua-5.3, exprtk, hashtk) — 261 files, 5.16 MB checked into the repo. No version pinning, no update path, no security audit trail.

v2 needs managed dependencies with: SHA-pinned versions, reproducible builds, cross-platform support, and CMake integration.

## Decision

**vcpkg in manifest mode** (`vcpkg.json` + git submodule at `third_party/vcpkg/`). Baseline pinned to a specific commit SHA for reproducibility.

Phase 1 dependencies: `fmt`, `spdlog`, `catch2`. Later phases add: `simdjson`, `sol2`/`lua`, `mimalloc`, `xxhash`, `blake3`, etc.

## Alternatives considered

- **Conan 2** — mature, good CMake integration, but requires Python runtime and `conanfile.py`. Heavier setup for contributors.
- **Nix flake** — excellent reproducibility but niche; most contributors won't have Nix. Will add as optional in Phase 7.
- **FetchContent only** — no binary caching, slow CI, no transitive dependency resolution.
- **Keep vendoring** — rejected for all reasons in Context.

## Consequences

- **Positive:** `vcpkg.json` is the single source of truth for deps. `cmake --preset debug` fetches everything automatically.
- **Positive:** Binary caching in CI via GitHub Actions cache.
- **Positive:** `builtin-baseline` SHA pins exact versions across all platforms.
- **Negative:** `third_party/vcpkg/` submodule adds ~50 MB to fresh clone (mitigated by `--depth 1`).
- **Negative:** First build fetches + compiles deps (~3-4 min cold). Cached builds are instant.

## References

- `docs/recon/deps.md` — vendored libs inventory.
- vcpkg manifest mode: https://learn.microsoft.com/en-us/vcpkg/users/manifests
