# ADR-0013 — Native filesystem watcher (inotify + polling fallback)

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

ADR-0008 deferred native filesystem watching to a future phase, using
polling (recursive directory walk comparing mtimes) as the portable
baseline. Polling works but has a constant CPU cost (~1 ms per scan per
100 files) and a latency floor equal to the poll interval.

For large sites (10 K+ files) or on battery-powered laptops, event-driven
watching is measurably better: zero CPU between changes, sub-50 ms
detection latency.

## Decision

**Implement platform-native filesystem watching with inotify on Linux,
stubs for kqueue (macOS) and ReadDirectoryChangesW (Windows), and a
polling fallback for unsupported platforms.**

### Architecture

- `NativeFileWatcher` class with the same `WatchCallback` interface as
  the existing `FileWatcher`.
- Compile-time platform detection via preprocessor:
  - `__linux__` → inotify with recursive directory monitoring.
  - `__APPLE__` → kqueue stub (currently falls through to polling).
  - `_WIN32` → ReadDirectoryChangesW stub (currently falls through to polling).
  - Other → polling fallback.
- `detect_best_backend()` returns the selected backend at runtime for
  diagnostic output.

### Linux inotify implementation

- Recursive watch: on `start()`, walk all directories under root and add
  individual inotify watches.
- Dynamic tracking: `IN_CREATE` on directories triggers new watch addition;
  `IN_DELETE_SELF` removes the watch.
- Event mask: `IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM |
  IN_MOVED_TO | IN_DELETE_SELF | IN_ATTRIB`.
- Event loop uses `select()` with 200 ms timeout for clean shutdown
  checking.
- 64 KB event buffer per read cycle.
- Ignored paths filtered via configurable substring list.

### Server integration

- `DevServer` now prefers `NativeFileWatcher` when available.
- Falls back to the original `FileWatcher` (polling) if native
  initialization fails (e.g., inotify fd limit reached).
- Backend name logged at startup for diagnostics.

## Alternatives considered

- **Use a cross-platform library (efsw, libuv)** — adds dependency weight;
  efsw is reasonable but its CMake integration conflicts with our vcpkg
  setup. Rolling our own keeps the dependency tree lean.
- **fanotify (Linux)** — requires `CAP_SYS_ADMIN`, not suitable for
  unprivileged dev environments.
- **FSEvents (macOS)** — better than kqueue for recursive watching but
  requires Objective-C. Deferred until macOS profiling shows a need.
- **Keep polling only** — rejected; CPU waste is unacceptable for large
  sites and laptop dev workflows.

## Consequences

- **Positive:** Sub-50 ms change detection on Linux.
- **Positive:** Near-zero CPU between filesystem events.
- **Positive:** Same callback interface — no changes needed in `DevServer`
  rebuild logic.
- **Positive:** Graceful fallback to polling if native API is unavailable.
- **Negative:** inotify watch limit (`/proc/sys/fs/inotify/max_user_watches`)
  can be hit on very large monorepos; documented in troubleshooting guide.
- **Negative:** macOS and Windows native backends are stubs — still use
  polling on those platforms until future phases.

## References

- ADR-0008 (dev server: cpp-httplib + polling watcher) — this ADR
  supersedes the polling-only decision for Linux.
- `crates/server/watcher/native_watcher.cpp` — inotify + polling fallback.
- `docs/sse-livereload.md` — end-to-end livereload flow.
