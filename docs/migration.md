# Migration guide: Nift v1 → v2

Nift v2 is a from-scratch rewrite. The CLI and project layout look
familiar, but several internals changed. This guide walks through the
move.

## TL;DR

```bash
nift migrate /path/to/your/v1-project
cd /path/to/your/v1-project
nift build
```

The migrator is non-destructive — your v1 files stay put. To revert,
delete `nift.json`.

## What gets translated

The v1 `siteInfo/nsm.config` (or `.nsm/nsm.config`, `nsm.config`) keys
map to v2's `nift.json` fields:

| v1 key | v2 field | Notes |
|---|---|---|
| `siteName` / `name` | `name` | identical |
| `version` | `version` | identical |
| `contentDir` / `contentDirectory` | `content_dir` | path reroots |
| `siteDir` / `outputDir` | `output_dir` | path reroots |
| `templateDir` / `templateDirectory` | `template_dir` | path reroots |
| `defaultTemplate` | `default_template` | identical |
| `buildDir` | `cache_dir` | semantics changed: now BLAKE3 cache |

Unknown v1 keys are reported by `nift migrate` rather than silently
dropped. You can then decide whether to re-author them as a v2
construct (or as a plugin).

`siteInfo/tracked.json` is copied to `.nift/tracked.json`.

## Directive changes

Most directives carry over identically:

- `@if` / `@else` / `@elif` / `@for` / `@while` — unchanged
- `@def` / `@set` / `@input` / `@write` — unchanged
- `@content` — unchanged
- `$var` / `${expr}` substitutions — unchanged

Notable changes:

- **`@for` separator**: `@for(i, 0, 3)` and `@for(i; 0; 3)` both parse.
  v1 only supported the comma form.
- **Comments**: line comments (`// …`) are stripped along with the
  trailing newline (consistent with v1, but now spec'd).
- **Lua blocks**: `@lua{ … }` or `@lua(file.lua)` — the bridge is now
  sol2-based, so Lua 5.4 with full standard library access (sandboxed
  by default; opt-in to `io`/`os` via `enable_unsafe()`).

If you have a directive v2 doesn't support, the recommended path is to
write a plugin (see [plugin-author.md](plugin-author.md)). The plugin
handler matches by directive name and runs at render time.

## Cache layout

v1's tracked data lived in `siteInfo/tracked.json`. v2 uses
`.nift/cache/` with two files:

- `.nift/cache/build_cache.json` — BLAKE3 hash + mtime per source
- `.nift/tracked.json` — migrated copy of v1's `tracked.json`

Both are JSON. You can `cat` them safely. Phase 8+ may switch to CBOR
for performance.

## Performance differences

In rough numbers (1k-page synthetic site, gcc -O2, 8-core x86_64):

- Cold build: ~ 2.4× faster (mmap + SIMD scan + work-stealing)
- Warm build (no changes): near-instant — cache short-circuits everything
- Single-file change rebuild: ~ 2-3× faster than v1's full rebuild

The Lua bridge add-on is **slightly slower** for tiny scripts because
sol2 has overhead the v1 vendored Lua API didn't. For non-trivial
scripts the difference vanishes; for trivial ones the overhead is sub-ms.

## What's unchanged

- Project directory layout (`content/`, `templates/`, `public/`)
- Directive surface — most v1 templates parse identically
- The `nift` (formerly `nsm`) command-style invocation

## What's removed (intentionally)

- Vendored third-party sources in `legacy/`. v2 reads them as a reference
  but does not link them. vcpkg owns dependencies now.
- v1's hand-written Makefile. v2 uses CMake + Ninja + presets.

## Reporting issues

If `nift migrate` output flags an unknown v1 key, please open an issue
with the key + value so we can decide whether to add it to the
translator or document the v2 alternative.

Repository: https://github.com/ronggothelast/nsm-v2
