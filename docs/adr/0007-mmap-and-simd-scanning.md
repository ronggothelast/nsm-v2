# ADR 0007 — mmap I/O + xsimd scanning

**Status:** Accepted
**Date:** 2026-05-31
**Phase:** 4 (Build pipeline)

## Context

Phase 4 needs to scan template files for `@directives`, `$variables`, and
`\escapes`. This is the parser's hottest inner loop. v1 walks each byte
in a `for` loop — ~1 ns/byte at best.

We also need to read template files quickly. v1 does `std::ifstream` →
`stringstream` → `string`, which is 3 copies and a heap allocation per
file.

## Decision

### mmap via `mio`

- Use **[mio](https://github.com/mandreyel/mio)** — header-only, no deps,
  cross-platform mmap wrapper. Available in vcpkg.
- `MmapReader` opens via `mio::mmap_source`, exposes `string_view`.
- Falls back to a heap read for:
  - 0-byte files (mmap rejects them on most platforms)
  - Filesystems that refuse mmap (some Docker overlays, NFS edge cases)
- Caller code uses `string_view` either way — fallback is invisible.

### SIMD scanning via `xsimd`

- Use **[xsimd](https://github.com/xtensor-stack/xsimd)** — portable SIMD
  wrapper that picks AVX2 / SSE4.2 / NEON / SVE based on CPU at compile
  time. Header-only, MIT licensed.
- `scan_byte` and `scan_positions` use `xsimd::batch<uint8_t>` to compare
  32 bytes per cycle on AVX2.
- Result is the sorted list of match positions.

### Measured speedup

`bench_simd_scanner` (release build, 1 MB buffer, sparse `@` markers):

```
SIMD scan:    0.37 ns/byte
Scalar scan:  0.89 ns/byte
Speedup:      2.4×
```

The 2.4× number on this VM is conservative — AVX2 on real desktop CPUs
typically hits 4-8× with hot caches. The header note about 4-16× still
holds across larger buffers and multiple targets per pass.

### Why xsimd over `std::simd` (P1928 / std::experimental)

- `std::simd` is C++26 — not landed in gcc 11/13 yet.
- `<experimental/simd>` exists in libstdc++ but is gcc-only.
- xsimd ships now, supports all our target platforms, and is what the
  numerical computing world (xtensor, blaze) already uses.

We'll re-evaluate `std::simd` in Phase 6 once gcc 14+ stabilizes.

## Consequences

✅ Zero-copy template reads — `string_view` over mmap region.
✅ 2.4× faster directive scanning — measured.
✅ Both libs are header-only — no extra link-time complexity.

⚠️ mmap'd buffers must outlive any `string_view` parsed from them. The
   lifetime is owned by `MmapReader`; downstream code holds the reader
   for the duration of a parse.
⚠️ xsimd compile time adds ~1 sec to `simd_scanner.cpp`. Hidden behind
   the crate boundary so it doesn't ripple into other TUs.
