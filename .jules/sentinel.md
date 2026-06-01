# Sentinel Journal — Nift v2

## 2026-06-01 — Path traversal in dev server, weakly_canonical alone insufficient

**Vulnerability:** `crates/server/http/http_server.cpp::resolve_static` built the
filesystem path with `root.str() + url_path` and never validated containment.
A `GET /../<secret>.txt` against a server bound on the project parent dir
returned HTTP 200 with the secret body. Confirmed exploitable: stashing the
fix and re-running the new test produced `200 != 200` with the secret body
visible in `res->body`.

**Learning:** The first instinct was to call `weakly_canonical` and prefix-match,
but two pitfalls bit:
  1. `weakly_canonical` resolves against an existing path prefix and silently
     drops trailing `..` segments. If the resolver later appended `.html` (the
     "clean URLs" fallback), the canonical form could differ from what was
     read from disk. Defense-in-depth requires rejecting `..` segments
     **before** the file lookup, not just after.
  2. String prefix comparison (`canon_candidate.starts_with(canon_root)`) is
     unsafe: `/srv/site` is a prefix of `/srv/site-evil/`. Use path-iterator
     containment instead — compare component by component.

  Also: percent-decoding must run before `..` rejection. `%2e%2e` would slip
  through any literal `..` filter applied to the raw URL.

**Prevention:** When adding any HTTP route that maps URLs to disk paths in
this repo, the four-layer check is mandatory:
  1. Reject NUL bytes.
  2. Percent-decode the URL.
  3. Reject any decoded segment equal to `..` (treat both `/` and `\` as
     separators on cross-platform code paths).
  4. After resolving, `weakly_canonical` both root and candidate, then walk
     path components to confirm containment.

The QA suite already had `tests/qa/network/test_server_security.py` ready to
catch a regression of this exact bug — keep that suite wired into CI.
