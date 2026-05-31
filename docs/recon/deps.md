# Dependency Graph & Vendored Libs

## Core include graph (Nift own headers only)

DAG, **0 cycles**. Layers from leaves up:

```
Layer 0 (no internal deps):
  ConsoleColor.h   Consts.h   Expr.h   Filename.h   Lolcat.h
  NumFns.h   Quoted.h   StrFns.h   SystemInfo.h   Timer.h

Layer 1:
  Directory.h    → Quoted
  Filename.h     → Quoted
  FileSystem.h   → Path, SystemInfo
  Lua.h          → StrFns
  Title.h        → ConsoleColor, Quoted
  RapidJSON.h    → FileSystem

Layer 2:
  Path.h         → ConsoleColor, Directory, Filename, SystemInfo
  Variables.h    → NumFns, Path, StrFns
  Pagination.h   → Path
  Getline.h      → ConsoleColor, Consts, FileSystem, Lolcat, StrFns
  GitInfo.h      → ConsoleColor, FileSystem
  ExprtkFns.h    → Consts, Expr, FileSystem, Quoted, Variables
  WatchList.h    → FileSystem, RapidJSON

Layer 3:
  LuaFns.h       → ConsoleColor, Consts, ExprtkFns, FileSystem, Lua, Path, Quoted, Variables
  TrackedInfo.h  → Path, Quoted, Title

Layer 4 (TOP — fattest):
  Parser.h       → DateTimeInfo, Expr, ExprtkFns, Getline, Lua, LuaFns,
                   Pagination, RapidJSON, SystemInfo, Timer, TrackedInfo, Variables

Layer 5:
  ProjectInfo.h  → GitInfo, Parser, Timer, WatchList

Layer 6 (entry):
  nsm.cpp        → GitInfo, ProjectInfo
```

**Observation:** the graph is well-layered; the proposed crate split in `AGENT_PROMPT.md` (`core / parser / runtime / project / build / server / cli / plugin / compat`) maps cleanly onto these layers. No restructuring fights required.

## Vendored libraries → replacement plan

| Vendored dir | Files | What it is | Replacement (v2) |
|---|---:|---|---|
| `rapidjson/` | 38 | RapidJSON SAX/DOM | **simdjson** (parse) + RapidJSON (write) via vcpkg, OR drop fully for `glaze` |
| `LuaJIT/` | 100+ | LuaJIT 2.1 amalgam | vcpkg `luajit` package, or `sol2` + system Lua |
| `Lua-5.3/` | 30+ | PUC Lua 5.3 | vcpkg `lua` 5.4 |
| `exprtk/` | 1 | Header-only expr eval | keep — header-only, pin via FetchContent |
| `hashtk/` | 2 | Hash toolkit (custom) | replace with **BLAKE3** + **xxHash3** (both vcpkg) |

ADRs to write in Phase 1:

- `0001-build-system-cmake-presets.md`
- `0002-dependency-manager-vcpkg-manifest.md`
- `0004-json-parser-simdjson.md`
- `0005-lua-runtime-sol2.md`
- `0006-hash-blake3-xxh3.md`

## Why this matters

- Replacing 5.16 MB of vendored code with package-managed equivalents cuts repo size by ~90 % and gets us security updates for free.
- Header-only `exprtk` is the only legitimate vendor — keep but version-pin.
- `hashtk` is the only fully custom dep; replacement with BLAKE3/xxH3 also unlocks the incremental cache strategy in Phase 4.
