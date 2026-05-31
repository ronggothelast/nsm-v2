# LOC & File Size Report

Generated: Phase 0 recon. Source: `legacy/` snapshot.

## Headline numbers

| Bucket | Files | Size | LOC |
|---|---:|---:|---:|
| **Core Nift code** | 54 | 845 KB | 32 629 |
| **Vendored (will replace)** | 261 | 5.16 MB | ~145 000 |
| **Total snapshot** | 315 | 5.98 MB | ~177 600 |

Vendored buckets: `rapidjson/`, `LuaJIT/`, `Lua-5.3/`, `exprtk/`, `hashtk/`.

## Core files — top 20 by size

| Size | LOC | File | Notes |
|---:|---:|---|---|
| **446.3 KB** | **16 949** | `Parser.cpp` | **MONOLITH** — split in Phase 2 |
| 144.8 KB | 5 278 | `ProjectInfo.cpp` | Split in Phase 3 (state machine refactor) |
| 66.9 KB | 2 374 | `nsm.cpp` | CLI entry — gut & rewrite in Phase 6 |
| 26.5 KB | 1 196 | `Getline.cpp` | Custom REPL line editor — keep as `crates/repl/` |
| 26.0 KB | 1 166 | `LuaFns.cpp` | Lua bridge — move to `crates/runtime/` |
| 17.1 KB | 719 | `FileSystem.cpp` | → `crates/core/fs/` |
| 14.8 KB | 393 | `Parser.h` | Header for monolith — split in Phase 2 |
| 12.1 KB | 533 | `ExprtkFns.h` | exprtk integration — `crates/runtime/expr/` |
| 10.5 KB | 498 | `Variables.cpp` | `crates/core/vars/` |
| 8.0 KB | 312 | `Lolcat.cpp` | Color output — `crates/core/term/` |
| 6.8 KB | 273 | `WatchList.cpp` | `crates/project/watch/` |
| 5.9 KB | 296 | `Path.cpp` | `crates/core/path/` |
| 5.5 KB | 164 | `ProjectInfo.h` | header — split with `.cpp` |
| 4.1 KB | 152 | `GitInfo.cpp` | `crates/project/git/` |
| 3.2 KB | 176 | `StrFns.cpp` | `crates/core/str/` |
| 3.0 KB | 151 | `SystemInfo.cpp` | `crates/core/sys/` |
| 2.8 KB | 195 | `NumFns.cpp` | `crates/core/num/` |
| 2.7 KB | 109 | `Quoted.cpp` | `crates/core/str/quoted/` |
| 2.1 KB | 103 | `FixIndenting.cpp` | absorb into `crates/parser/format/` |
| 2.1 KB | 119 | `DateTimeInfo.cpp` | `crates/core/time/` |

Remaining 34 files are < 2 KB each (small helpers, headers).

## Parser.cpp anatomy

`Parser.cpp` alone = 51 % of all core LOC. Function count: 100+ member functions.
Notable callsites (line numbers in legacy file):

- `Parser::run` — line 828 (~170 lines)
- `Parser::build` — line 1001 (~427 lines)
- `Parser::n_read_and_process` / `_fast` — lines 1428, 1443, 1454
- `Parser::f_read_and_process` / `_fast` — lines 1713, 1728
- `Parser::read_and_process_fn` — line 1903 (**12 562 lines** until next function at 14 465) — the directive dispatch megaswitch
- `Parser::set_var_from_str` — line 14 934 (~184 lines)
- `Parser::read_func_name` — line 15 194 (~145 lines)
- `Parser::read_def` — line 15 339 (~462 lines)
- `Parser::read_block` — line 16 512 (~201 lines)
- `Parser::read_else_blocks` — line 16 713

The 12 K-line `read_and_process_fn` is the single largest unit and must be re-expressed as a directive table + visitor in Phase 2. ADR `0003-parser-modularization.md` will record the chosen pattern.
