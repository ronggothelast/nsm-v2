# Phase 8 — Performance + DX + Template Inheritance

## Goal
Upgrade dev server to native FS watch + WebSocket livereload, add CBOR cache, structured plugin args, and template inheritance.

## Tasks

### 8.1 — Native filesystem watcher (replace polling)
- [ ] `crates/server/watcher.hpp` — platform-dispatched watcher
- [ ] Linux: `inotify` wrapper (IN_CREATE, IN_MODIFY, IN_DELETE, IN_MOVED_TO)
- [ ] macOS: `FSEvents` wrapper (kFSEventStreamCreateFlagFileEvents)
- [ ] Windows: `ReadDirectoryChangesW` wrapper
- [ ] Fallback: polling (current behavior) for unsupported platforms
- [ ] Update `FileWatcher` API to use new backend
- [ ] Tests: mock FS events, verify callback delivery
- [ ] ADR 0011

### 8.2 — WebSocket livereload (replace polling)
- [ ] Integrate lightweight WebSocket server (uWebSockets or standalone)
- [ ] `/__nift/ws` endpoint for livereload
- [ ] File watcher triggers WS push → client reloads
- [ ] Update client JS (`/__nift/livereload.js`) to use WebSocket
- [ ] Fallback: keep polling for proxied environments
- [ ] Tests: WS connect, message delivery, fallback
- [ ] ADR 0012

### 8.3 — CBOR cache format
- [ ] Add `tinycbor` or `nlohmann-json` CBOR support
- [ ] `BuildCache::save_cbor()` / `BuildCache::load_cbor()`
- [ ] Auto-detect format (JSON vs CBOR) on load
- [ ] Migration: first CBOR save converts from JSON
- [ ] Benchmark: CBOR vs JSON read/write at 1k, 10k entries
- [ ] Tests: round-trip, migration, format detection
- [ ] ADR 0013

### 8.4 — Plugin structured args
- [ ] Extend `NiftPluginVtable` — `render` receives structured args
- [ ] Args schema: `{ "key": "value", "flag": true, "count": 42 }`
- [ ] Backward compat: plugins with ABI v1 get raw string
- [ ] ABI version bump to 2
- [ ] Update plugin-author.md
- [ ] Tests: structured args round-trip, v1 fallback
- [ ] ADR 0014

### 8.5 — Template inheritance
- [ ] New directives: `@extends("layout.html")`, `@section("name")...@endsection`
- [ ] `@yield("name")` in layouts — renders child section
- [ ] `@parent` in sections — append to parent's section content
- [ ] Nested inheritance (layout extends base layout)
- [ ] Parser: add `ExtendsNode`, `SectionNode`, `YieldNode`, `ParentNode`
- [ ] Evaluator: resolve inheritance chain before rendering
- [ ] Error: circular extends detection
- [ ] Tests: basic extends, nested, yield, parent, circular error
- [ ] ADR 0015
- [ ] Update docs

## Order
8.1 → 8.2 → 8.3 → 8.4 → 8.5

## Verification
- All existing 415+ tests pass
- New tests for each feature
- CI green on all platforms
- Tag: `v2-phase-8-performance`
