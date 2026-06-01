# Plugin author guide

Add custom directives to Nift v2 by writing a dynamic library that
implements the C ABI in `crates/plugin/include/nift/plugin/plugin_abi.h`.

## Minimal example

`my_plugin.cpp`:

```cpp
#include <cstdlib>
#include <cstring>
#include <nift/plugin/plugin_abi.h>

namespace {

const char* kDirectives[] = {"upper"};

NiftPluginInfo info = {
    .name = "upper",
    .version = "0.1.0",
    .abi_version = NIFT_PLUGIN_ABI_VERSION,
};

NiftPluginDirectives directives = {
    .names = kDirectives,
    .count = 1,
};

char* render(const char* /*name*/, const char* args, char** /*err*/) {
  std::string out(args);
  for (auto& c : out) c = std::toupper(static_cast<unsigned char>(c));
  char* result = static_cast<char*>(std::malloc(out.size() + 1));
  std::memcpy(result, out.c_str(), out.size() + 1);
  return result;
}

void free_result(char* s) { std::free(s); }

NiftPluginVtable vtable = {
    .abi_version = NIFT_PLUGIN_ABI_VERSION,
    .info = &info,
    .directives = &directives,
    .render = &render,
    .free_result = &free_result,
};

}  // namespace

extern "C" const NiftPluginVtable* nift_plugin_init() { return &vtable; }
```

Build it as a shared library:

```bash
g++ -std=c++17 -fPIC -shared \
    -I /path/to/nsm-v2/crates/plugin/include \
    -o libupper.so my_plugin.cpp
```

## Loading

Drop `libupper.so` (or `.dylib` on macOS, `.dll` on Windows) into your
project's `plugins/` directory. Then in your content:

```html
<p>@upper(hello world)</p>
```

…will render as:

```html
<p>HELLO WORLD</p>
```

## Contract

- `nift_plugin_init()` is called **once** at host startup; cache the
  vtable in static storage.
- Returned strings from `render` must be heap-allocated. The host calls
  `free_result` to release them; never return a pointer the host
  shouldn't free.
- Set `abi_version = NIFT_PLUGIN_ABI_VERSION` exactly. If your plugin
  was compiled against an older version, the host refuses to load it
  with a clear error.
- The `render` function may be called from multiple threads
  concurrently. If you keep state, protect it with a mutex.
- Failure path: return `NULL` and (optionally) set `*err_out` to a
  heap-allocated error message. The host calls `free_result` on it too.

## Versioning

`NIFT_PLUGIN_ABI_VERSION` only bumps on **breaking** ABI changes. Adding
new optional fields to `NiftPluginVtable` happens at the same major
version (consumers see the old size; new fields default to NULL/zero
because the host treats them as optional).

## Why C ABI?

C++ has no stable ABI across compilers (libstdc++ vs libc++, MSVC vs
clang-cl, exceptions, RTTI alignment). A pure C interface lets you ship
a plugin compiled with **any** compiler and have it load cleanly into a
host built with **any other**. The vtable struct is C-layout, so even
Rust or Zig plugins compose.

## Future ABI extensions (planned, not yet committed)

- `render_structured` — pass parsed args (key/value pairs) instead of a
  raw string.
- `init` / `shutdown` — let plugins allocate per-host state.
- `query_capability` — discover optional features at load time.

These will arrive at `NIFT_PLUGIN_ABI_VERSION = 2` (with v1 still loadable).
