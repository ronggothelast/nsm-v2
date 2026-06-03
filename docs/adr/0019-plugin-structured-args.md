# ADR-0019 — Plugin ABI v2: structured key-value arguments

**Status:** Accepted
**Date:** 2026-06-04
**Phase:** 8 (Polish & Advanced Features)
**Deciders:** lead architect agent

---

## Context

ADR-0009 established the plugin C ABI (v1) with a string-based render
function: `char* render(const char* name, const char* args, char** err_out)`.
The `args` parameter is a raw string that the plugin must parse itself.

This works for simple directives (`@greeting`) but breaks down for
plugins that need typed, named parameters:

```
@codeblock(lang="rust", linenos=true, highlight="3,5-8")
```

With ABI v1, the plugin receives the raw string
`lang="rust", linenos=true, highlight="3,5-8"` and must implement its
own parser — error-prone and inconsistent across plugins.

## Decision

**Introduce ABI v2 with a structured argument interface: plugins receive
pre-parsed key-value pairs via `NiftPluginArg*` array.**

### New render function signature

```c
typedef char* (*NiftPluginRenderStructuredFn)(
    const char* name,
    const NiftPluginArg* args,
    size_t arg_count,
    char** err_out
);
```

Where `NiftPluginArg` is:

```c
typedef struct {
    const char* key;    // Argument name (e.g., "lang")
    const char* value;  // Argument value (e.g., "rust")
} NiftPluginArg;
```

### Backward compatibility

- **ABI v1 plugins continue to work unchanged.** The host accepts both
  v1 (`abi_version = 1`) and v2 (`abi_version = 2`) plugins.
- If `render_structured` is `NULL` in the vtable, the host falls back
  to the string-based `render` function, serializing args as a raw
  string (same as v1).
- v2 plugins may set `render` to `NULL` if they only implement
  `render_structured`, but both can be provided for maximum
  compatibility.

### Argument parsing (host side)

- The template parser extracts key-value pairs from directive arguments.
- Supports: `key="value"`, `key='value'`, `key=value`, `key=true/false`,
  `key=123`.
- Positional arguments (no key) are passed with key set to their index
  as a string (e.g., `"0"`, `"1"`).
- The parsed array is passed to the plugin — no parsing burden on the
  plugin side.

### Vtable changes

```c
typedef struct {
    uint32_t abi_version;           // 1 or 2
    const NiftPluginInfo* info;
    const NiftPluginDirectives* directives;
    NiftPluginRenderFn render;              // Required for v1, optional for v2
    NiftPluginFreeFn free_result;           // Always required
    NiftPluginRenderStructuredFn render_structured;  // NEW in v2, optional
} NiftPluginVtable;
```

## Alternatives considered

- **JSON arguments** — pass args as a JSON string, plugin parses with
  its own JSON library. More flexible (nested objects, arrays) but
  forces every plugin to link a JSON parser.
- **WASM plugins** — sandboxed, portable, but significantly more complex
  host/plugin interface. Deferred to a future ABI version.
- **Protobuf / FlatBuffers for args** — overkill for key-value pairs.
  Schema management adds friction for plugin authors.
- **Break ABI v1** — rejected. Backward compatibility is a hard
  requirement; existing v1 plugins must continue to work.

## Consequences

- **Positive:** Plugins receive clean, pre-parsed arguments — no
  string-parsing boilerplate.
- **Positive:** Consistent argument handling across all plugins.
- **Positive:** Fully backward compatible — no migration required for
  existing v1 plugins.
- **Positive:** Simple C struct — trivial to implement in any language
  with C FFI.
- **Negative:** Values are strings only — no native int/bool/float types.
  Plugins must convert as needed. Acceptable for v2; a future v3 could
  add typed values.
- **Negative:** No support for nested structures (arrays, objects).
  Sufficient for flat key-value directive arguments.

## References

- ADR-0009 (CLI design + plugin C ABI + v1 migrator) — this ADR
  extends the plugin ABI.
- `docs/plugin-author-v2.md` — plugin author guide for ABI v2.
- `crates/plugin/plugin_abi.h` — ABI header with v2 definitions.
