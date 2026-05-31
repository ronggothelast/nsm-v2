# ADR 0008 — Dev server: cpp-httplib + polling watcher

**Status:** Accepted
**Date:** 2026-05-31
**Phase:** 5 (Dev server + asset pipeline)

## Context

Phase 5 needs a dev server that:

1. Serves the build output directory over HTTP.
2. Reloads the browser when files change.
3. Has zero external service dependencies (no nginx, no node).
4. Compiles with the project — no separate build pipeline.

## Decision

### HTTP layer: `cpp-httplib`

Use **[cpp-httplib](https://github.com/yhirose/cpp-httplib)**:

- Single header. Available via vcpkg as `cpp-httplib`.
- MIT licensed.
- Supports HTTP/1.1, HTTPS (when linked with OpenSSL — opt-in).
- Built-in static file routing isn't quite flexible enough for our
  livereload injection, so we register a catch-all `Get(".*")` handler
  and resolve files manually with a small MIME table.

### HTTP/2 deferred

The spec calls for HTTP/2, but `cpp-httplib` is HTTP/1.1 only. HTTP/2
would require pulling in `nghttp2` + a TLS stack and managing certs.
Not worth the complexity for a localhost dev server. Phase 6 will
revisit if a strong need emerges (e.g. testing PWA service workers).

### Hot reload: poll-based generation token

Instead of WebSockets:

- `GET /__nift/livereload` returns the current "generation" token (a
  monotonically increasing integer, bumped via `notify_rebuild()`).
- Client-side JS at `/__nift/livereload.js` polls every 500 ms.
- When the token changes → `window.location.reload()`.

Why polling over WebSockets:

- No socket-upgrade handshake — works through any proxy.
- Simpler to test deterministically (just GET the endpoint).
- 500 ms latency is fine for the dev workflow.
- WebSockets would require pulling more of httplib's optional code.

The script is auto-injected into HTML responses before `</body>` (case
insensitive). Non-HTML responses pass through unchanged.

### File watcher: polling

`FileWatcher` walks the project directory tree and tracks mtimes,
firing `Added` / `Modified` / `Removed` events on changes. Polling
is portable (Linux/macOS/Windows) and dead simple.

Native filesystem APIs — `inotify`, `FSEvents`, `ReadDirectoryChangesW`
— are deferred to Phase 6 if profiling shows polling is the
bottleneck. For the typical ≤ 1000-file site, a 500 ms poll cycle is
imperceptible.

## Consequences

✅ Self-contained: no nginx, no node, no separate watcher process.
✅ HTTP server + watcher both portable across Linux/macOS/Windows.
✅ Hot reload works through any proxy (no WS upgrade).

⚠️ HTTP/1.1 only — Phase 6 may add HTTP/2 if needed.
⚠️ Polling watcher uses a small CPU baseline (~ 1 ms per scan per
   100 files). Acceptable for dev. Native APIs in Phase 6.
