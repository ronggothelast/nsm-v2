/// @file plugin_loader.hpp
/// @brief Dynamic-library plugin loader.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "nift/core/path.hpp"
#include "nift/core/types.hpp"
#include "nift/plugin/plugin_abi.h"

namespace nift::plugin {

/// @brief A loaded plugin handle. Owns the dlopen handle and vtable.
class LoadedPlugin {
 public:
  ~LoadedPlugin();
  LoadedPlugin(const LoadedPlugin&) = delete;
  LoadedPlugin& operator=(const LoadedPlugin&) = delete;
  LoadedPlugin(LoadedPlugin&&) noexcept;
  LoadedPlugin& operator=(LoadedPlugin&&) noexcept;

  /// Plugin name (from vtable info).
  std::string_view name() const noexcept;

  /// Plugin version string.
  std::string_view version() const noexcept;

  /// Directives this plugin handles.
  std::vector<std::string> directives() const;

  /// Invoke the plugin's render function for `directive_name` with raw `args`.
  /// Returns the rendered string on success, or Error on failure.
  ::nift::Expected<std::string, ::nift::Error> render(std::string_view directive_name,
                                                      std::string_view args) const;

  /// Underlying ABI vtable (for advanced use).
  const NiftPluginVtable* vtable() const noexcept { return vtable_; }

 private:
  friend class PluginRegistry;
  LoadedPlugin();
  void* handle_ = nullptr;
  const NiftPluginVtable* vtable_ = nullptr;
};

/// @brief Plugin registry — loads + tracks plugins by directive.
class PluginRegistry {
 public:
  PluginRegistry() = default;
  ~PluginRegistry() = default;

  PluginRegistry(const PluginRegistry&) = delete;
  PluginRegistry& operator=(const PluginRegistry&) = delete;

  /// Load a plugin from a shared library path.
  /// On success, the plugin is registered for each of its declared directives.
  ::nift::Expected<std::monostate, ::nift::Error> load(
      const ::nift::core::Path& dll_path);

  /// True if a directive is handled by any loaded plugin.
  bool has_directive(std::string_view name) const;

  /// Render via the registered plugin for `name`. Error if no handler.
  ::nift::Expected<std::string, ::nift::Error> render(std::string_view directive_name,
                                                      std::string_view args) const;

  /// Number of plugins loaded.
  std::size_t size() const noexcept { return plugins_.size(); }

  /// Names of all loaded plugins.
  std::vector<std::string> names() const;

 private:
  std::vector<std::unique_ptr<LoadedPlugin>> plugins_;
  std::unordered_map<std::string, LoadedPlugin*> by_directive_;
};

}  // namespace nift::plugin
