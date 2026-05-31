/// @file plugin_abi.h
/// @brief Stable C ABI for nift plugins.
///
/// Plugins are dynamic libraries (.so / .dylib / .dll) implementing this
/// ABI. The ABI is intentionally minimal and pure C — plugins compiled
/// with one C++ compiler should load cleanly into a host built with
/// another. Versioning is explicit; the host refuses to load plugins
/// whose ABI version doesn't match.
///
/// To author a plugin in C++:
///
///   extern "C" const NiftPluginVtable* nift_plugin_init() { ... }
///

#ifndef NIFT_PLUGIN_ABI_H
#define NIFT_PLUGIN_ABI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define NIFT_PLUGIN_ABI_VERSION 1

typedef struct {
  /// Stable plugin name (UTF-8, NUL-terminated).
  const char* name;
  /// Plugin version string (semver, NUL-terminated).
  const char* version;
  /// ABI version this plugin was compiled against.
  uint32_t abi_version;
} NiftPluginInfo;

/// @brief Render-time hook. Called once per directive registered by the plugin.
///
/// `name` is the directive name (e.g. "highlight" → matched by `@highlight(...)`).
/// `args` is the raw argument string (NUL-terminated UTF-8).
/// On success, return a heap-allocated NUL-terminated C string. The host calls
/// `free_result` to release it.
///
/// On failure, return NULL and set *err_out to a NUL-terminated message
/// (also released via `free_result`).
typedef char* (*NiftPluginRenderFn)(const char* name, const char* args,
                                    char** err_out);

/// @brief Free a string previously returned by `render` or via `err_out`.
typedef void (*NiftPluginFreeFn)(char* str);

/// @brief List of directive names this plugin handles.
/// `count` is the number of names; `names` is an array of NUL-terminated strings.
typedef struct {
  const char* const* names;
  size_t count;
} NiftPluginDirectives;

typedef struct {
  uint32_t abi_version;            ///< Must equal NIFT_PLUGIN_ABI_VERSION
  const NiftPluginInfo* info;
  const NiftPluginDirectives* directives;
  NiftPluginRenderFn render;
  NiftPluginFreeFn free_result;
} NiftPluginVtable;

/// @brief Plugin entry point. Each plugin DLL must export this.
typedef const NiftPluginVtable* (*NiftPluginInitFn)();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // NIFT_PLUGIN_ABI_H
