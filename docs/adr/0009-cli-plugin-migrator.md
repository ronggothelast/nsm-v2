# ADR 0009 — CLI design + plugin C ABI + v1 migrator

**Status:** Accepted
**Date:** 2026-05-31
**Phase:** 6 (CLI + plugin + migrator)

## Context

Phase 6 needs three independent pieces of user-facing surface:

1. The `nift` CLI binary that wires Phases 1–5 into a tool.
2. A plugin system so third parties can add directives without forking.
3. A migrator that takes a Nift v1 (`nsm`) project layout to v2 cleanly.

## Decision

### CLI: hand-rolled argparse

We don't pull in CLI11 or argparse. The flag surface is small, the
parser is < 100 lines, and the dep tree already stretches across
ten libraries. The argparse supports:

- `--flag=value` and `--flag value`
- Bool flags from a declared list (`--quiet`, `--no-cache`, …)
- Short flags including bundled bools (`-qV`)
- `--` to stop flag parsing

Subcommands: `init`, `build`, `serve`, `migrate`, `clean`, `version`,
`help`. `serve` does an initial build, starts the HTTP server, and
attaches a `FileWatcher` that triggers rebuilds + a livereload bump.

### Plugin C ABI

Plugins are dynamic libraries exposing a single C entry point:

```c
const NiftPluginVtable* nift_plugin_init();
```

The vtable carries a stable `abi_version` (currently 1), a name + version
struct, an array of directive names, a render callback, and a free
function for returned heap strings.

Why C ABI:

- A plugin compiled with one C++ compiler/stdlib must load into a host
  built with another. C++ ABI is famously not stable across compilers.
- The interface surface is small enough that pure C is no burden.
- Loading uses `dlopen`/`LoadLibrary` directly — no third-party loader.

The host's `PluginRegistry` rejects any plugin whose `abi_version` doesn't
match. Future ABI bumps are explicit.

### v1 → v2 migrator

`crates/compat/` reads a v1 `nsm.config` (in `siteInfo/`, `.nsm/`, or root)
and translates known keys to v2's `nift.json`. Unknown keys are recorded
in the report rather than silently dropped — the user gets visibility.
Source content + template directories are read in place (v2 reads them
unchanged), so the migrator is non-destructive: it never deletes v1
files. The user can revert by deleting `nift.json`.

`tracked.json` is also copied to `.nift/tracked.json` if present.

## Consequences

✅ The `nift` binary is now usable end-to-end:
   `nift init && nift build && nift serve`.
✅ Plugins are isolated from C++ ABI churn — once compiled, they keep
   loading across host upgrades unless we bump `NIFT_PLUGIN_ABI_VERSION`.
✅ v1 migration is one command: `nift migrate`.

⚠️ Hand-rolled argparse may need refactoring if we add a lot of flags;
   move to a library if the surface grows past 25 flags.
⚠️ The plugin ABI commits us to passing strings only. Phase 7 may
   extend with structured args (currently only the raw `args` string).
