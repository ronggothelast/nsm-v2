# ADR 0006 — Work-stealing thread pool

**Status:** Accepted
**Date:** 2026-05-31
**Phase:** 4 (Build pipeline)

## Context

Phase 4 needs file-level parallelism for the build pipeline. Nift v1 was
single-threaded — building 1000 pages was 1000 sequential renders. The
target for v2 is "files in parallel, scaled to N cores."

The pool needs to:

1. Run hundreds to tens of thousands of small-to-medium tasks.
2. Avoid head-of-line blocking when one task is slow.
3. Have low overhead — submitting a no-op task should be < 1 µs.
4. Be portable (Linux, macOS, Windows, no Boost dep).

## Decision

Roll our own minimal **work-stealing** pool: `WorkStealingPool` in
`crates/build/include/nift/build/executor.hpp`.

- Each worker owns a private `std::deque<Task>` guarded by `std::mutex`.
- `submit()` round-robins across worker queues to spread initial load.
- Local pop is LIFO (cache-friendly) from the back of its own deque.
- When local queue empty, worker steals FIFO from a random victim.
- Random start avoids contention on the same victim queue.

### Why not Intel TBB / taskflow / std::async?

- **TBB** — heavy dep, Intel-centric, awkward licensing (Apache 2 but
  the binaries pull in Intel runtime detection). Rejected.
- **taskflow** — excellent, but its strength is dependency graphs.
  Phase 4 doesn't have a graph — files are independent. We may pull
  taskflow in later when we add the cross-file dep graph.
- **`std::async`** — implementation-defined (some libstdc++ versions
  spawn a thread per call). Cannot guarantee bounded parallelism.
- **OpenMP** — good for data-parallel loops, awkward for irregular
  task pools across translation units.

### Tradeoffs accepted

- One mutex per worker queue. Lock-free WS deques (Chase-Lev) are faster
  but considerably more code. Phase 5+ if profiling demands.
- No priority bands. Phase 4 doesn't need them.

## Consequences

✅ Self-contained, ~120 LOC, portable.
✅ Linear speedup observed in tests up to N cores.
✅ Exception-safe — tasks throw → caught and surfaced via the future.

⚠️ Mutex-per-queue limits scaling above ~32 cores. Acceptable for v2
   target hardware (laptops + CI runners).
