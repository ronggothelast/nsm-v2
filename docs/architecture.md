# Architecture

Nift v2 is split into independent crates, each with a tight responsibility.

```
                ┌─────────────────────┐
                │  apps/nift (binary) │
                └──────────┬──────────┘
                           │
                ┌──────────▼──────────┐
                │     nift::cli       │
                │ argparse + commands │
                └─┬─────────┬───┬─────┘
                  │         │   │
       ┌──────────┘         │   └─────────┐
       │                    │             │
┌──────▼──────┐  ┌──────────▼─────┐  ┌────▼────────┐
│ nift::compat│  │  nift::build   │  │ nift::server│
│  v1→v2 port │  │  pipeline +    │  │  HTTP +     │
└──────┬──────┘  │  exec + scan   │  │  watcher    │
       │         └───┬────────┬───┘  └──────┬──────┘
       │             │        │             │
       │   ┌─────────▼──┐  ┌──▼──────────┐  │
       │   │nift::parser│  │nift::project│  │
       │   │ lex+parse+ │  │ config +    │  │
       │   │ ast+eval   │  │ build cache │  │
       │   └─────┬──────┘  └──────┬──────┘  │
       │         │                │         │
       │         └────────┬───────┘         │
       │                  │                 │
       │     ┌────────────▼──────────┐      │
       └────►│      nift::core       │◄─────┘
             │ Path / FS / Str /     │
             │ Time / Sys / Result   │
             └───────────────────────┘
```

## Crate boundaries

| Crate | Owns | Depends on |
|---|---|---|
| `nift::core` | `Path`, `Result`/`Expected`, FS, Str, DateTime | nothing (stdlib + fmt) |
| `nift::parser` | Lexer, parser, AST, evaluator | core |
| `nift::runtime` | Lua bridge (sol2), expression eval | core |
| `nift::project` | `ProjectConfig`, `BuildCache` (BLAKE3 + CBOR/JSON) | core |
| `nift::build` | Pipeline, executor, mmap, SIMD scanner | core, parser, project |
| `nift::server` | HTTP server, native file watcher, SSE livereload | core |
| `nift::compat` | v1 → v2 migrator | core, project |
| `nift::plugin` | C-ABI dynamic plugin loader (v1 + v2) | core |
| `nift::markdown` | GFM parser (cmark-gfm), front matter, code highlighting | core |
| `nift::tailwind` | Tailwind CSS integration (subprocess wrapper) | core |
| `nift::assets` | CSS minify, JS bundle, BLAKE3 fingerprinting | core |
| `nift::images` | Image optimization: resize, WebP/AVIF, responsive srcset | core |
| `nift::cli` | argparse + subcommands | all of the above |
| `apps/nift` | Main binary | `nift::cli` |

The DAG is strict — no back-edges, no cycles. `core` knows nothing
about anything else; `cli` sits on top.

## Build pipeline

`nift::build::Pipeline::build(sources)` orchestrates per-file work:

```
for each source file:
  ┌──► Hash content (BLAKE3)
  │
  ├──► Cache check
  │      ├──► hit  ──► skip (cached)
  │      └──► miss ──► continue
  │
  ├──► mmap the source
  │
  ├──► SIMD scan for @ markers (xsimd, AVX2/SSE4.2/NEON)
  │
  ├──► Parse to AST (parser crate)
  │
  ├──► Evaluate (parser crate evaluator + runtime expr)
  │
  ├──► Write output to public/<rel-path>
  │
  └──► Update cache entry
```

All work is dispatched into a `WorkStealingPool`. Workers steal from
random victims when their local deque drains. Cache lookups are
serialised at the boundary; rendering is fully parallel.

### Why work-stealing

A site is a bag of mostly-independent files. The work imbalance is real
(some pages have heavy `@for` / Lua, others are static HTML), so
fixed-chunk parallelism stalls on the slow ones. Work-stealing keeps
all workers busy until the last file finishes.

### Why mmap + SIMD

Profile of v1 showed ~30% of build time inside the parser's linear search
for `@`. mmap eliminates one read syscall + one userspace copy. xsimd
batch-scans 32 bytes per AVX2 op (16 per SSE4.2, 16 per NEON). Measured
2.4× speedup on a 1 MB synthetic buffer (`bench_simd_scanner`).

## Parser & evaluator

The parser is context-aware: in **text mode**, only `@` and `$` (and
their escape sequences) are special. Inside a directive's parens or
braces, structural tokens (`,`, `}`, etc.) are recognised. This matches
Nift v1's behaviour where surrounding HTML is pass-through.

The AST uses `std::shared_ptr<Node>` (not `unique_ptr`) — this works
around `std::variant`'s requirement that alternatives be copyable on
older compilers (gcc 11.4) while keeping ownership clear.

The evaluator is a visitor over the AST. It can call into:

- `nift::runtime::evaluate_expr` for `@if(...)`, `@while(...)` conditions
- `nift::runtime::LuaRuntime` for `@lua{ ... }` blocks
- `nift::plugin::PluginRegistry::render` for plugin-handled directives

## Build cache

`nift::project::BuildCache` persists incremental build state in
`.nift/cache/` as both CBOR (`index.json.cbor`) and JSON (`index.json`).
On rebuild:

1. Hash current source (BLAKE3).
2. Compare against cached hash + mtime.
3. If both match and the output exists, skip.
4. Otherwise rebuild and update.

`load()` tries CBOR first (faster, ~40% smaller), falls back to JSON.
`save()` writes both — JSON is kept for human readability and backward
compatibility with older Nift v2 versions.

## Plugin C ABI

Plugins are dynamic libraries exposing one symbol:

```c
const NiftPluginVtable* nift_plugin_init();
```

The vtable carries an `abi_version` (currently 2), name + version,
declared directives, a `render` function pointer, a `render_structured`
hook for key-value args (ABI v2), and a free callback for returned heap
strings. The host (`nift::plugin::PluginRegistry`) loads via
`dlopen`/`LoadLibrary`, validates `abi_version` (accepts both v1 and v2),
and routes matching directives to the plugin.

C ABI was chosen so plugins compiled with one compiler/stdlib load into
a host built with another. C++ ABI churn would otherwise force a recompile
on every host upgrade.

## Concurrency model

- The `WorkStealingPool` runs N workers (default: hardware concurrency).
- Each worker has a private LIFO deque (cheap, locality-friendly).
- Steals are FIFO from a random victim (avoids contention on hot deques).
- Synchronisation: per-deque mutex; futures convey results.
- The `BuildCache` is single-threaded (mutated only between scheduling
  rounds). The hashing is parallel.

## What we deferred

- HTTP/2 in the dev server (cpp-httplib is HTTP/1.1 only).

## Phase 8 crates

Phase 8 added four new crates that extend the build pipeline:

```\
│  ┌──────────────────────┐  ┌──────────────────┐
│  │    nift::markdown    │  │  nift::tailwind   │
│  │  cmark-gfm, front    │  │  Tailwind CSS     │
│  │  matter, code hl     │  │  auto-config,     │
│  │                      │  │  class scanning,  │
│  │  depends: core       │  │  subprocess       │
│  │           cmark-gfm  │  │                   │
│  └──────────────────────┘  │  depends: core    │
│                            └──────────────────┘
│  ┌──────────────────────┐  ┌──────────────────┐
│  │    nift::assets      │  │   nift::images    │
│  │  CSS minify          │  │  resize, WebP,    │
│  │  (lightningcss),     │  │  AVIF, responsive │
│  │  JS bundle (esbuild),│  │  srcset,          │
│  │  BLAKE3 fingerprint  │  │  <picture> tags   │
│  │                      │  │                   │
│  │  depends: core       │  │  depends: core    │
│  └──────────────────────┘  └──────────────────┘
```

| Crate | Owns | Depends on |
|---|---|---|
| `nift::markdown` | GFM parser (cmark-gfm), YAML/JSON front matter, code highlighting (14 languages) | core |
| `nift::tailwind` | Auto-config generation, class scanning, subprocess Tailwind CLI compilation | core |
| `nift::assets` | CSS minification (lightningcss), JS bundling (esbuild), BLAKE3 content fingerprinting | core |
| `nift::images` | Image resize, WebP/AVIF conversion, responsive srcset generation, `<picture>` HTML | core |

### Dev server enhancements

The dev server (Phase 8) gained:
- **Native filesystem watcher**: inotify on Linux with recursive directory
  monitoring, polling fallback for macOS/Windows. Auto-detects best backend.
- **SSE livereload**: `/__nift/livereload` endpoint using Server-Sent Events
  (text/event-stream), replacing the Phase 5 polling approach. Client JS
  auto-detects SSE support, falls back to polling transparently.
