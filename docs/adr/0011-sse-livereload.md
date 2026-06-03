# ADR-0011 — SSE livereload replacing polling

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

ADR-0008 established a poll-based livereload mechanism: the client JS
fetches `GET /__nift/livereload` every 500 ms, compares a generation token,
and reloads on change. This works but introduces up to 500 ms latency
between file save and browser refresh, and generates unnecessary HTTP
requests when nothing has changed.

Phase 8 aims to deliver instant feedback in the dev workflow. SSE
(Server-Sent Events) provides push-based notification over plain HTTP
with no library upgrades, no binary protocol, and automatic reconnection
built into every browser's `EventSource` API.

## Decision

**Replace polling with SSE as the primary livereload transport**, retaining
polling as an automatic fallback when SSE is unavailable.

### Server side

- `GET /__nift/livereload` now returns `text/event-stream` with chunked
  transfer encoding.
- On connection: emits `data: connected`.
- On file change: emits `data: reload` to all connected clients.
- Implementation uses `cpp-httplib`'s chunked streaming — no new
  dependencies.

### Client side

- The injected `/__nift/livereload.js` script first attempts an
  `EventSource` connection.
- On `onmessage` → `window.location.reload()`.
- On `onerror` → close `EventSource`, fall back to polling
  `/__nift/livereload/poll` at 500 ms intervals.

### Polling fallback retained

- `GET /__nift/livereload/poll` still returns the generation token as
  plain text.
- Used only when SSE is blocked (rare proxy configs).

## Alternatives considered

- **WebSocket** — full-duplex, but requires upgrade handshake, more
  complex server code, and some proxies strip upgrade headers. SSE is
  simpler for a unidirectional server→client signal.
- **Long-polling** — works everywhere but adds latency per reconnect cycle
  and higher server memory for held-open requests. SSE is more efficient.
- **Keep polling only** — rejected because 500 ms latency is noticeable
  during rapid edit-save-refresh cycles.

## Consequences

- **Positive:** Sub-50 ms reload latency on modern browsers.
- **Positive:** Zero wasted HTTP requests when nothing changes.
- **Positive:** Automatic reconnection handled natively by `EventSource`.
- **Positive:** Works through nginx, Apache, Cloudflare, Docker, AWS ALB
  without configuration.
- **Positive:** No new dependency — `cpp-httplib` already supports
  chunked streaming.
- **Negative:** Slightly more complex client JS (~25 lines vs ~10).
- **Negative:** SSE connections hold an HTTP socket open; negligible for
  a local dev server with 1–2 browser tabs.

## References

- ADR-0008 (dev server + polling watcher) — this ADR extends it.
- `docs/sse-livereload.md` — protocol specification.
- `crates/server/livereload_sse.cpp` — SSE endpoint implementation.
