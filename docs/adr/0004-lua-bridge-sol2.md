# ADR 0004 — Lua bridge: sol2

**Status:** Accepted
**Date:** 2026-05-31
**Phase:** 3 (Runtime)

## Context

Nift v1 vendored Lua 5.3 + LuaJIT directly into the source tree and called
the C API by hand. That approach worked but has known issues:

- Manual `lua_State*` lifetime management — easy to leak or mismanage stack.
- Boilerplate `lua_pushstring`/`lua_tostring` everywhere.
- Hard to add type-safe bindings between C++ and Lua.
- Vendored deps complicate updates and licensing audits.

For Phase 3 we needed a Lua integration that:

1. Sandboxes by default (no fs/io/os exposure to template authors).
2. Has a clean, modern C++ API (RAII, type-safe).
3. Plays well with vcpkg-managed Lua.
4. Adds zero or near-zero runtime cost compared to raw C API.

## Decision

Use **[sol2](https://github.com/ThePhD/sol2)** as the C++ binding for Lua.

- Header-only, available via vcpkg as `sol2`.
- MIT licensed.
- Mature (used by Sol Compositor, Spine, Disney, Roblox, etc).
- Wraps `lua_State*` in `sol::state` with RAII.
- Type-safe set/get for Lua globals.
- `safe_script` returns `protected_function_result` — exceptions/errors are
  propagated cleanly, no manual `lua_pcall` boilerplate.

We pair sol2 with vcpkg's stock `lua` package (Lua 5.4). LuaJIT can be
re-enabled later via a CMake option if benchmarks demand it.

## Sandbox model

`LuaRuntime` opens only safe libraries by default:

- `base`, `string`, `table`, `math`

`io`, `os`, `package`, `debug`, `coroutine` are **excluded** unless a
caller explicitly opts in via `enable_unsafe()`. This matches the
threat model — Nift templates run in build pipelines and may execute
untrusted snippets pulled from content authors.

## Alternatives considered

- **Raw lua.h C API** — keeps zero-dep simplicity, but every binding becomes
  20 lines of stack manipulation. Rejected.
- **LuaBridge** — older, less actively maintained. Rejected.
- **kaguya** — abandoned. Rejected.
- **lua-intf** — narrower scope than sol2. Rejected.
- **Embed LuaJIT directly** — performance win but loses portability
  (no native macOS-arm64, no Windows MSVC support). Deferred to Phase 4
  as opt-in.

## Consequences

✅ Clean, type-safe Lua bindings.
✅ vcpkg manages Lua 5.4 + sol2 — no vendored sources.
✅ Default sandbox prevents accidental host I/O.
✅ Easy to add user-defined types (registry, etc.) in Phase 4+.

⚠️ sol2 includes are large (~50 KB of templates). Compile time for files
that include `<sol/sol.hpp>` is non-trivial — we hide it behind the `LuaImpl`
PIMPL idiom so only `lua_runtime.cpp` pays the cost.
