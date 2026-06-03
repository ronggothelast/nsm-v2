# Plugin Author Guide

This guide explains how to write native plugins for Nift v2 using the C ABI plugin system.

## Overview

Plugins are dynamic libraries (`.so` / `.dylib` / `.dll`) that export a single entry point:

```c
extern "C" const NiftPluginVtable* nift_plugin_init();
```

The host (Nift) loads the library, calls `nift_plugin_init()`, and uses the returned vtable to invoke your plugin's render function.

## Quick Start

### Minimal Plugin

```c
// my_plugin.c
#include "plugin_abi.h"
#include <stdlib.h>
#include <string.h>

static char* render(const char* name, const char* args, char** err_out) {
  const char* result = "<h1>Hello from plugin!</h1>";
  char* out = strdup(result);
  return out;
}

static void free_result(char* str) { free(str); }

static const char* directive_names[] = { "greeting" };
static const NiftPluginDirectives directives = {
  .names = directive_names,
  .count = 1
};

static const NiftPluginInfo info = {
  .name = "my-plugin",
  .version = "1.0.0",
  .abi_version = 2
};

static const NiftPluginVtable vtable = {
  .abi_version = 2,
  .info = &info,
  .directives = &directives,
  .render = render,
  .free_result = free_result,
  .render_structured = NULL  // Use string-based render
};

extern "C" const NiftPluginVtable* nift_plugin_init() {
  return &vtable;
}
```

### Build

```bash
# Linux
g++ -shared -fPIC -o my_plugin.so my_plugin.c

# macOS
clang++ -shared -o my_plugin.dylib my_plugin.c

# Windows (MSVC)
cl /LD my_plugin.c /Fe:my_plugin.dll
```

### Usage

```
@greeting
```

Outputs: `<h1>Hello from plugin!</h1>`

## ABI v2 Structured Args

ABI v2 adds optional structured arguments — key-value pairs instead of raw strings.

### Structured Render Function

```c
static char* render_structured(
    const char* name,
    const NiftPluginArg* args,
    size_t arg_count,
    char** err_out
) {
  for (size_t i = 0; i < arg_count; i++) {
    if (strcmp(args[i].key, "lang") == 0) {
      // Handle "lang" argument
    }
  }
  // ...
}

// In vtable:
static const NiftPluginVtable vtable = {
  .abi_version = 2,
  .info = &info,
  .directives = &directives,
  .render = render,
  .free_result = free_result,
  .render_structured = render_structured  // NEW in v2
};
```

### Backward Compatibility

- ABI v1 plugins continue to work — host accepts both v1 and v2
- If `render_structured` is NULL, host falls back to string-based `render`
- ABI v2 is fully backward compatible

## ABI Contract

| Field | Type | Description |
|-------|------|-------------|
| `abi_version` | `uint32_t` | Must be 1 or 2 |
| `info` | `NiftPluginInfo*` | Plugin name, version, ABI version |
| `directives` | `NiftPluginDirectives*` | List of directive names |
| `render` | `NiftPluginRenderFn` | String-based render (required) |
| `free_result` | `NiftPluginFreeFn` | Free strings returned by render |
| `render_structured` | `NiftPluginRenderStructuredFn` | Structured render (optional, v2+) |

### Render Function

```c
typedef char* (*NiftPluginRenderFn)(
    const char* name,      // Directive name
    const char* args,      // Raw argument string
    char** err_out         // Error message (or NULL on success)
);
```

- Return: heap-allocated NUL-terminated string (freed via `free_result`)
- On failure: return NULL, set `*err_out` to error message

### Error Handling

```c
static char* render(const char* name, const char* args, char** err_out) {
  if (invalid_input) {
    *err_out = strdup("Invalid argument");
    return NULL;
  }
  // ...
}
```

## Versioning

- Plugin ABI version must match host's `NIFT_PLUGIN_ABI_VERSION`
- Host currently accepts ABI v1 and v2
- Future ABI versions will be backward compatible where possible
- Plugin version is independent of ABI version

## Safety

- Plugins run in the host process (no sandbox)
- Plugin code has full access to system resources
- Host calls `free_result` on all returned strings
- Host never calls `free` directly on plugin-allocated memory
