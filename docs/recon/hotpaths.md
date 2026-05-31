# Hot Paths — where Nift spends its time

Static analysis from `Parser.cpp` (446 KB, 16 949 LOC). Dynamic profile (perf record + flamegraph) deferred to Phase 4 once a working build exists — recorded here as **TODO-PHASE-4**.

## Candidate hot paths (static, by call-site density and line span)

| Rank | Function | Loc range | Why hot |
|---:|---|---|---|
| 1 | `Parser::read_and_process_fn` | 1903 – 14 465 (**12 562 lines**) | Directive dispatch megaswitch. Called per `@`-directive on every template. The biggest win in the rewrite. |
| 2 | `Parser::n_read_and_process_fast` | 1443 / 1454 | "Fast path" already singled out by original author → known hot. Two overloads, suggests partial specialization need. |
| 3 | `Parser::f_read_and_process_fast` | 1728 | File-level analogue of (2). |
| 4 | `Parser::build` | 1001 (~427 LOC) | Per-page build orchestrator. Parallelization target in Phase 4. |
| 5 | `Parser::skip_whitespace` | 14 465 | Tight inner loop — `inline`-marked → already perf-sensitive in v1. SIMD candidate. |
| 6 | `Parser::read_def` | 15 339 (~462 LOC) | Variable definition parser, runs on every `@def`. |
| 7 | `Parser::set_var_from_str` | 14 934 (~184 LOC) | Type-coercion hot spot for variable assignments. |
| 8 | `Parser::replace_var` / `replace_vars` | 14 690 / 14 758 | String substitution across template body. SIMD scan target. |
| 9 | `Parser::read_block` / `read_else_blocks` | 16 512 / 16 713 | Conditional block extraction. |
| 10 | `find_last_of_special` | 5 (free fn) | Character class scan — replace with SIMD `xsimd` lookup. |

## Planned optimizations (Phase 4)

- **SIMD `@`-directive scan**: replace per-character loop in `read_and_process_fn` with 16-byte SWAR or `std::experimental::simd` scan → 4-16× scan throughput.
- **mmap + `string_view`**: stop copying template bodies; operate zero-copy on mmap'd input.
- **Bump arena for AST**: per-template arena, freed at end-of-page → eliminates per-node `new`/`delete`.
- **Work-stealing builder**: parallelize page builds; current `Parser::build` is per-page serial.
- **Persistent incremental cache**: BLAKE3 hash of `(template + deps + vars)` → skip unchanged pages.
- **`mimalloc` as default allocator**: free 10-25 % across the board.
- **String interning** for path / variable names (massive duplication observed in `read_def` + `replace_var` codepaths).

## Benchmark methodology (Phase 4)

Reference workloads:
1. `examples/blog-1000-posts/` — throughput cold.
2. `examples/blog-1000-posts/` — incremental (1 file changed).
3. `examples/portfolio-deep-nesting/` — deep `@include` recursion.
4. `examples/lua-heavy/` — Lua script eval.
5. Real-world: build of `nift.dev` site itself.

Compare v2 wall-clock and RSS vs `legacy/` binary built from the same `Makefile`. Acceptance: v2 ≥ v1 across all five. Stretch: 2-5×.
