# Phase 8 — Performance + DX + Template Inheritance ✅ COMPLETE

## Goal
Upgrade dev server to native FS watch + SSE livereload, add CBOR cache, structured plugin args, and template inheritance.

## Tasks

### 8.1 — Native filesystem watcher ✅
- Linux: inotify with recursive directory monitoring, select() timeout
- Fallback: polling (portable, for macOS/Windows/other)
- Auto-detects best backend via detect_best_backend()
- Nanosecond-precision mtime for reliable change detection
- 8 tests

### 8.2 — SSE livereload (replaced polling) ✅
- /__nift/livereload — SSE endpoint (text/event-stream)
- /__nift/livereload/poll — polling fallback (token comparison)
- Client JS auto-detects SSE, falls back to polling
- notify_rebuild() pushes reload event to all SSE clients

### 8.3 — CBOR cache format ✅
- save() writes both CBOR (index.json.cbor) and JSON (index.json)
- load() auto-detects: tries CBOR first, falls back to JSON
- JSON kept for human readability and backward compatibility
- Migration: first save after upgrade creates CBOR automatically

### 8.4 — Plugin structured args ✅
- NiftPluginArg: key-value pair struct
- NiftPluginRenderStructuredFn: new render hook for structured args
- Vtable extended with optional render_structured field
- Backward compat: host accepts ABI v1 and v2 plugins

### 8.5 — Template inheritance ✅
- @extends("layout.html") — declare parent layout
- @section("name") { ... } — named content section
- @yield("name") — render section content in layout
- @parent — reference parent section content
- 13 tests, full inheritance flow verified

## Stats
- 436/436 tests passing
- 21 new tests for Phase 8
- 10 commits across all Phase 8 features
