# Risks, Anti-Patterns & Migration Blockers

Catalog of issues identified during Phase 0 read-through. Each row gets a mitigation tracked in the corresponding phase.

## 🔴 Critical (blocks v2 release if unaddressed)

| Risk | Detail | Mitigation | Phase |
|---|---|---|---|
| **`Parser.cpp` size** | 446 KB / 16 949 LOC / single function spans 12 562 lines | Split into `crates/parser/{lexer,ast,evaluator,directives/*}` with visitor pattern; snapshot tests vs v1 fixtures | Phase 2 |
| **No tests at all** | Zero unit, zero integration, zero golden | Build harness from scratch (Catch2 + custom golden runner). Target 85 % coverage before any rewrite | Phase 1 |
| **C++11 idioms throughout** | Raw `new`/`delete`, manual `char*`, no `string_view`, no `std::filesystem` | Modernize per-crate as we touch it. Keep `compat/` layer for translation back to v1 ABI when needed | Phases 1-6 |
| **No backwards-compat plan** | Existing v1 project YAML / template syntax not yet specified for v2 | `nift compat` mode + `nift migrate` tool, both validated against 10+ real v1 projects | Phase 6 |

## 🟠 High (degrades quality, address before phase tag)

| Risk | Detail | Mitigation |
|---|---|---|
| **Vendored deps unmanaged** | rapidjson, LuaJIT, Lua-5.3, exprtk, hashtk all checked in; no SHAs, no update path | Phase 1: replace with vcpkg manifest, pin SHA-256 per dep |
| **No CI** | Manual `make` only; no platform matrix | Phase 1: GitHub Actions matrix (Linux gcc 13/14 + clang 17/18, macOS, Windows MSVC + MinGW, FreeBSD, WASM) |
| **`-Werror` disabled** | Comment in Makefile shows author tried then backed out | v2: `-Werror` always-on; fix every warning before merge |
| **No format / lint config** | Inconsistent indent, brace style across files | Phase 1: `.clang-format` + `.clang-tidy` checked in; CI fails on diff |
| **Lua bundling fragile** | Recursive `make` into `Lua-5.3/src/` and `LuaJIT/src/` | Replace with vcpkg / system pkg-config |

## 🟡 Medium (won't block but cleanup)

| Risk | Detail | Plan |
|---|---|---|
| `nsm.cpp` 2 374-line entry | All CLI parsing inline | Phase 6: replace with proper argparse + subcommands |
| `Getline.cpp` 1 196 lines | Custom readline-style | Keep but isolate as `crates/repl/` — optional, may swap for `replxx` |
| `WatchList` uses RapidJSON for state | OK in v1 | Phase 3: persist build cache in CBOR (smaller, faster) — keep JSON option |
| No `.editorconfig` / `.gitattributes` | LF/CRLF inconsistency likely | Phase 1: add both, normalize repo |

## 🟢 Low (note for posterity)

| Risk | Detail |
|---|---|
| `Lolcat.cpp` decorative output | Keep — it's part of Nift personality |
| `Title.cpp`, `Quoted.cpp` tiny helpers | Fold into `crates/core/str/` |
| `Pagination.cpp` 19 LOC | Probably stub; investigate before deciding crate placement |

## Non-goals (out-of-scope for v2.0.0)

- Web UI / dashboard
- Cloud-hosted build service
- VS Code extension (community-driven, post-launch)
- Theme marketplace

These will go into `docs/non-goals.md` formally after Phase 1.

## Open questions for user

- **Brand**: keep "Nift" name? Upstream maintainer (Nicholas Ham) hasn't been pinged yet. If hard-fork required, propose codename **`nift-next`** with attribution.
- **License**: MIT confirmed (`legacy/LICENSE` is MIT 2015-present Nicholas Ham). v2 stays MIT.
- **Telemetry**: opt-in OpenTelemetry traces — confirm acceptable?
