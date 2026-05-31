/// @file lua_runtime.hpp
/// @brief Lua scripting bridge for Nift v2 templates.
///
/// Embeds Lua via sol2 (header-only modern C++ wrapper). Provides a
/// sandboxed execution context per template — no fs/io/os access by default.

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "nift/core/types.hpp"

namespace nift::runtime {

/// Forward declaration — hides sol::state implementation detail.
class LuaImpl;

/// @brief Sandboxed Lua execution context.
///
/// Each instance owns its own `lua_State*`. By default, only safe stdlib
/// modules are loaded: `string`, `table`, `math`. fs/io/os are excluded
/// unless explicitly enabled via `enable_unsafe()`.
class LuaRuntime {
 public:
  LuaRuntime();
  ~LuaRuntime();

  LuaRuntime(const LuaRuntime&) = delete;
  LuaRuntime& operator=(const LuaRuntime&) = delete;
  LuaRuntime(LuaRuntime&&) noexcept;
  LuaRuntime& operator=(LuaRuntime&&) noexcept;

  /// Execute a Lua snippet, return stdout-style result as string.
  /// On error, returns Error with line/column info.
  ::nift::Expected<std::string, ::nift::Error> exec(std::string_view code);

  /// Set a Lua global variable from a string.
  void set_global(std::string_view name, std::string_view value);

  /// Get a Lua global variable as a string. Empty if not set or not stringable.
  std::string get_global(std::string_view name) const;

  /// Enable unsafe stdlib (io/os/package/debug). Disabled by default.
  void enable_unsafe();

 private:
  std::unique_ptr<LuaImpl> impl_;
};

}  // namespace nift::runtime
