# 📚 Legacy — Nift v1 Source (Reference Only)

Snapshot dari [`nifty-site-manager/nsm`](https://github.com/nifty-site-manager/nsm) commit terakhir per 2024-01-13.

**JANGAN edit file di folder ini.** Read-only reference untuk:
- Snapshot tests (output v2 harus byte-identical untuk fixtures yang sama)
- Algorithm reference (parsing logic, Lua bridge, dll)
- Migration validation (`nift migrate` harus convert project v1 → v2 dengan zero data loss)

## File besar yang perlu di-split di v2

| File | Size | Action di v2 |
|---|---|---|
| `Parser.cpp` | 456 KB | Split jadi `crates/parser/` modular (lexer, AST, evaluator, directives/) |
| `ProjectInfo.cpp` | 148 KB | Split jadi `crates/project/` |
| `nsm.cpp` | 68 KB | Move ke `apps/nift/` + extract CLI parsing ke `crates/cli/` |
| `LuaFns.cpp` | 27 KB | `crates/runtime/lua.cpp` |
| `Getline.cpp` | 27 KB | `crates/core/getline.cpp` |

## Vendored deps (akan diganti FetchContent / vcpkg di v2)

- `Lua-5.3/` — replace with system Lua via vcpkg
- `LuaJIT/` — replace with vcpkg port
- `exprtk/` — header-only, keep but vendor via FetchContent
- `rapidjson/` — replace with `simdjson` (faster) or vcpkg rapidjson
- `hashtk/` — replace with `xxhash` / `blake3` via vcpkg

## Upstream

https://github.com/nifty-site-manager/nsm (MIT, Nicholas Ham 2015-present)
