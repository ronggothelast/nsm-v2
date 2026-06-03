# ADR-0017 — Asset pipeline: lightningcss, esbuild, BLAKE3 fingerprinting

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

A production SSG must deliver optimized assets: minified CSS, bundled
JS, and cache-busted filenames. Nift v2 currently copies static assets
as-is. Phase 8 adds a full asset pipeline that processes CSS and JS
during the build and produces fingerprinted output files.

## Decision

**Implement a three-stage asset pipeline: CSS minification via
lightningcss, JS bundling via esbuild, and BLAKE3 content-hash
fingerprinting for all processed assets.**

### Stage 1: CSS minification (lightningcss)

- **lightningcss** (Rust-based CSS processor, available as a standalone
  binary and C library).
- Invoked as a subprocess: `lightningcss --minify --bundle input.css`.
- Handles: minification, vendor prefixing, nesting, CSS modules.
- Replaces the need for PostCSS + cssnano + autoprefixer chains.
- If lightningcss is not installed, CSS is copied as-is with a warning.

### Stage 2: JS bundling (esbuild)

- **esbuild** (Go-based JS bundler, available as a standalone binary).
- Invoked as a subprocess: `esbuild input.js --bundle --minify --outdir=...`.
- Handles: bundling, tree-shaking, minification, JSX/TS transpilation.
- Dramatically faster than webpack/rollup (~10–100×).
- If esbuild is not installed, JS is copied as-is with a warning.

### Stage 3: BLAKE3 fingerprinting

- Every processed asset gets a content-hash suffix:
  `style.css` → `style.a3f2b1c9.css`.
- Hash is BLAKE3-256 (first 8 hex chars) of the final file content.
- HTML templates referencing assets are rewritten to use fingerprinted
  URLs (via a lookup table built during the asset pass).
- Matches the BLAKE3 choice from ADR-0005 (build cache).

### Pipeline orchestration

- Assets under `assets/` are processed in dependency order.
- Results are cached in `.nift/cache/assets/` keyed by input hash.
- Unchanged assets skip reprocessing (incremental builds).
- Pipeline runs in parallel with page rendering via the work-stealing
  pool (ADR-0006).

## Alternatives considered

- **Parcel** — zero-config bundler, but heavier and slower than esbuild.
  Also Node.js-native; subprocess invocation is more fragile.
- **SWC** — fast Rust-based transpiler, but its bundler is less mature
  than esbuild's. esbuild has broader ecosystem adoption.
- **csso / clean-css** for CSS minification — pure JS, slower than
  lightningcss, and lack autoprefixing.
- **SHA-256 fingerprinting** — slower than BLAKE3 for no security
  benefit in this context. BLAKE3 is already a project dependency.
- **No fingerprinting** — rejected; long-lived CDN caches require
  content-based busting.

## Consequences

- **Positive:** Production-quality asset output (minified, bundled,
  cache-busted) with zero user configuration.
- **Positive:** Incremental — unchanged assets skip processing.
- **Positive:** Parallel with page rendering — no sequential bottleneck.
- **Positive:** BLAKE3 fingerprinting reuses existing crypto dependency.
- **Negative:** Two optional external dependencies (lightningcss,
  esbuild) on the build machine. Gracefully degraded when absent.
- **Negative:** Subprocess invocation adds process-spawn overhead
  (~10–50 ms per invocation). Mitigated by caching.

## References

- ADR-0005 (BLAKE3 hashing) — reused for fingerprinting.
- ADR-0006 (work-stealing pool) — parallel execution.
- ADR-0016 (Tailwind integration) — Tailwind output feeds into this pipeline.
- lightningcss: https://lightningcss.dev/
- esbuild: https://esbuild.github.io/
