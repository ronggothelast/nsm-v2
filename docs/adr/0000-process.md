# ADR-0000 — Architecture Decision Record process

**Status:** Accepted
**Date:** Phase 0 (Recon)
**Deciders:** lead architect agent

---

## Context

Nift v1 has zero design documentation; every choice (Makefile structure, vendored deps, monolithic `Parser.cpp`, raw `new`/`delete` style) was made implicitly. The v2 rewrite spans 7 phases and dozens of crossroads — picking a JSON parser, picking a Lua binding, picking a build system, picking an allocator. We need a lightweight, durable way to record **why** each choice was made so future contributors don't re-litigate decisions or, worse, silently reverse them.

## Decision

Adopt the **Architecture Decision Record (ADR)** convention popularized by Michael Nygard. Each significant decision becomes a short markdown file in `docs/adr/` numbered sequentially.

### File naming

```
docs/adr/NNNN-short-kebab-title.md
```

`NNNN` is 4-digit zero-padded, monotonic. Numbers are **never reused**, even if the ADR is later superseded.

### Mandatory sections

```markdown
# ADR-NNNN — Title

**Status:** Proposed | Accepted | Superseded by ADR-XXXX | Deprecated
**Date:** Phase N (date or week)
**Deciders:** <names>

---

## Context
What's the situation? What constraint or problem forces a decision?

## Decision
What did we choose? State it crisply, top of section.

## Alternatives considered
- Option A — pros, cons, why rejected
- Option B — ...

## Consequences
- Positive: ...
- Negative: ...
- Follow-up work: ...

## References
- Spec section, related ADRs, external docs.
```

### When to write one

**Mandatory** for any of the following:
- Build system choice (CMake vs Bazel vs Meson).
- Dep manager choice (vcpkg vs Conan vs Nix).
- Replacing a vendored library with a package-managed equivalent.
- C++ standard version (C++23 vs C++20 fallback policy).
- Concurrency model (jthread vs TBB vs Taskflow).
- Allocator strategy (mimalloc, arenas, interning).
- ABI shape for plugins (C, C++, WASM).
- Breaking-change to user-visible config / CLI / template syntax.
- Anything affecting backwards compatibility with v1.

**Optional but encouraged:**
- Major refactor strategies (e.g., how the `Parser.cpp` split is structured).
- Performance trade-offs that prefer clever code over simple code.

### Lifecycle

1. **Proposed** when the PR introducing it opens — sit on it for review.
2. **Accepted** when the PR merges.
3. **Superseded** when a later ADR overrides it. The old ADR stays in the tree; only the `Status:` line and a footer pointer to the new ADR change.
4. **Deprecated** when the decision is no longer relevant but no replacement exists.

ADRs are **never deleted**. The history is part of the architecture.

## Alternatives considered

- **Wiki pages.** Rejected — drifts from code, no version history aligned with branches.
- **Long-form RFCs only.** Rejected — too heavy for routine decisions; we'd skip writing them.
- **Inline `// design:` comments.** Rejected — not greppable across the project, no review process.
- **GitHub Discussions only.** Rejected — not in-repo, search is weak.

## Consequences

- **Positive:** every meaningful choice has a paper trail; new contributors can answer "why?" without asking a maintainer.
- **Positive:** ADRs become the syllabus for Phase 7 docs (`docs/architecture/`).
- **Negative:** mild overhead — ~15 min per ADR.
- **Negative:** ADR count will hit 30-50 by v2.0.0; needs an index. We will maintain `docs/adr/README.md` listing all ADRs by number + status as the registry.

## References

- Michael Nygard, "Documenting Architecture Decisions" (2011).
- `AGENT_PROMPT.md` §EXECUTION PROTOCOL (mandates ADRs for "every pilihan besar").
