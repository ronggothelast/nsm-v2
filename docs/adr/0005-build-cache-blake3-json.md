# ADR 0005 — Build cache: BLAKE3 + JSON (CBOR deferred)

**Status:** Accepted
**Date:** 2026-05-31
**Phase:** 3 (Project)

## Context

Phase 3 requires a persistent incremental build cache so that re-runs of
`nift build` only regenerate files whose source content (or transitive
deps) changed.

The original spec called for BLAKE3 hashing + CBOR persistence.

## Decision

- **Hash function: BLAKE3** — keep as specified. Available via vcpkg
  (`blake3` package, target `BLAKE3::blake3`). Outperforms SHA-256 by
  ~10× and is cryptographically robust.
- **Persistence: nlohmann/json** — defer CBOR to Phase 4.

### Why JSON now, CBOR later

CBOR (`tinycbor`/`cbor-cpp`) gives ~30 % space savings and faster parse
times, but it has tradeoffs that don't pay off in Phase 3:

1. **Debugging UX.** During Phase 3 we want to inspect cache entries with
   `cat .nift/cache/index.json | jq` to verify hash propagation. CBOR
   needs `cbor2json` round-trips for the same workflow.
2. **Header-only convenience.** `nlohmann/json` is a single header, already
   used elsewhere in the project (planned for `nift.toml` parsing in
   Phase 6 via Tomlplusplus or a JSON config fallback).
3. **Migration is local.** `BuildCache::save()` and `BuildCache::load()`
   are the only two functions that touch serialization. Swapping JSON →
   CBOR is a single-file change behind a stable interface.

### Hash design

- `hash_content(s)` — BLAKE3-256 over input bytes, hex encoded (64 chars).
- `hash_file(p)` — read + hash. Surfaces `not_found`/`io_error` via Expected.
- `hash_combined(hashes)` — sort inputs lexicographically, hash with `\n`
  separator. Sort makes the result order-independent — important because
  we don't want spurious cache invalidation when dep traversal order
  changes between runs.

### Atomic save

`BuildCache::save()` writes to `index.json.tmp` then `std::filesystem::rename`s
into place. Crash-safe — interrupted runs leave the previous index intact.

## Schema (v1)

```jsonc
{
  "version": 1,
  "entries": [
    {
      "source": "content/post1.md",
      "output": "public/post1.html",
      "content_hash": "deadbeef…",  // 64 hex chars
      "deps_hash": "cafebabe…",
      "mtime": 1700000000,
      "built_at": 1700000123
    }
  ]
}
```

Bumping `version` field signals a breaking change → caches with mismatched
versions are discarded (`load()` returns empty cache).

## Consequences

✅ Crash-safe persistence via tmp + rename.
✅ Human-debuggable cache.
✅ BLAKE3 hashing meets perf/security goals.
✅ Schema is simple and versioned.

⚠️ JSON is ~3× larger than CBOR. Phase 4 migration if profiling shows
   parse/serialize is a bottleneck for large sites (>10 K pages).
