# ADR-0016 — Tailwind CSS integration via subprocess

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

Tailwind CSS is the dominant utility-first CSS framework. Users expect
SSGs to support it natively. The Tailwind ecosystem provides a
standalone CLI (`tailwindcss`) that scans source files, generates
utility-class CSS, and outputs a minified stylesheet.

Integrating Tailwind into the Nift build pipeline lets users write
`class="text-blue-500 p-4"` in their templates and get a production-ready
CSS file with zero manual configuration.

## Decision

**Integrate Tailwind CSS by invoking the Tailwind CLI as a subprocess
during the build pipeline.**

### Build flow

1. User includes a `tailwind.config.js` (or `.ts`) in the project root.
2. User provides an input CSS file (e.g., `assets/css/input.css`) with
   `@tailwind` directives.
3. During the asset pipeline phase, Nift invokes:
   ```
   tailwindcss -i assets/css/input.css -o .nift/cache/tailwind.css --minify
   ```
4. The output CSS is then processed by the asset pipeline (lightningcss
   minification, BLAKE3 fingerprinting — see ADR-0017).

### Detection

- If `tailwind.config.js` or `tailwind.config.ts` exists in the project
  root, Tailwind processing is automatically enabled.
- If no config file is found, Tailwind step is skipped (zero overhead).

### CLI resolution

- First checks `./node_modules/.bin/tailwindcss` (local install).
- Falls back to `tailwindcss` on `$PATH` (global install or standalone
  binary).
- If neither is found, emits a clear error message with installation
  instructions.

### Dev server

- In dev mode (`nift serve`), Tailwind CSS is regenerated on every
  rebuild cycle when source templates change.
- Tailwind CLI's `--watch` mode is NOT used — Nift's own file watcher
  triggers rebuilds and re-invokes the CLI as needed.

## Alternatives considered

- **Embed Tailwind as a library (C++ port)** — does not exist; Tailwind
  is JavaScript/TypeScript. Porting it would be a multi-year effort.
- **Require PostCSS / Node.js build pipeline** — forces users into a
  Node.js ecosystem they may not want. Subprocess approach is
  non-invasive.
- **Use UnoCSS instead** — compatible utility syntax, faster, written
  in JS but also has a Rust CLI. Rejected because Tailwind has
  overwhelming market share and user demand.
- **Ship Tailwind output as a static asset (no processing)** — defeats
  the purpose; users need per-project scanning and purging.

## Consequences

- **Positive:** Tailwind support with zero Nift code complexity —
  delegates all CSS logic to the official CLI.
- **Positive:** Users keep their existing `tailwind.config.js` — no
  Nift-specific configuration.
- **Positive:** Automatic detection means zero config for Tailwind
  users, zero overhead for non-Tailwind users.
- **Negative:** Requires Node.js (or the standalone Tailwind binary) on
  the build machine. Documented as an optional dependency.
- **Negative:** Subprocess invocation adds ~200–500 ms per rebuild cycle
  for Tailwind scanning. Acceptable for dev; mitigated in production
  builds by the incremental cache.

## References

- Tailwind CSS CLI: https://tailwindcss.com/docs/installation
- ADR-0017 (asset pipeline) — Tailwind output feeds into the pipeline.
