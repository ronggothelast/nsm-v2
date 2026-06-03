# ADR-0012 — Build cache: dual CBOR + JSON format

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

ADR-0005 chose JSON (via `nlohmann/json`) for build cache persistence and
explicitly deferred CBOR to a future phase. The rationale was debugging
convenience during Phase 3.

Now in Phase 8, with sites exceeding 10 K pages, profiling shows that
cache parse/serialize accounts for ~8 % of cold-start build time. CBOR
delivers ~3× smaller files and ~2× faster parse times. The ADR-0005
migration plan ("swap JSON → CBOR behind a stable interface") is now
executed — but rather than a hard cutover, we adopt a dual format with
auto-detection.

## Decision

**Support both CBOR and JSON for cache persistence, with CBOR as the
default write format and auto-detection on read.**

### Format negotiation

- `BuildCache::save()` writes CBOR by default.
- `BuildCache::load()` auto-detects format by inspecting the first byte:
  - CBOR maps start with `0xa0`–`0xbf` or `0xbf` (indefinite).
  - JSON objects start with `{` (0x7b).
- If a JSON cache is read, it is transparently upgraded to CBOR on the
  next save.
- Users can force JSON output via `--cache-format json` (for debugging).

### Library

- CBOR encoding/decoding via `cbor-cpp` (lightweight, header-only,
  available via vcpkg).
- `nlohmann/json` retained for the JSON fallback path and other
  non-cache uses in the project.

### Schema versioning

- Cache schema bumped to `version: 2`.
- Version 1 (JSON-only) caches are read but rewritten as version 2 CBOR.
- Unknown versions are discarded (same behavior as ADR-0005).

## Alternatives considered

- **Hard cutover to CBOR** — simpler code, but breaks `cat .nift/cache/index.json | jq`
  debugging workflow. Dual format preserves it.
- **MessagePack** — similar performance to CBOR but less standardized,
  no RFC, smaller ecosystem in C++.
- **FlatBuffers / Cap'n Proto** — zero-copy parsing is attractive but
  schema management overhead is excessive for a simple cache format.
- **Keep JSON only** — rejected; 10 K+ page sites show measurable
  slowdown.

## Consequences

- **Positive:** ~3× smaller cache files (important for CI artifact size).
- **Positive:** ~2× faster cache load on cold starts.
- **Positive:** JSON debugging workflow preserved via `--cache-format json`.
- **Positive:** Seamless upgrade — old JSON caches are read and auto-converted.
- **Negative:** Two serialization code paths to maintain and test.
- **Negative:** New dependency (`cbor-cpp`) added to vcpkg manifest.

## References

- ADR-0005 (build cache: BLAKE3 + JSON, CBOR deferred) — this ADR
  partially supersedes it (CBOR is no longer deferred).
- `crates/core/build_cache.cpp` — dual-format implementation.
- CBOR RFC 8949: https://www.rfc-editor.org/rfc/rfc8949
