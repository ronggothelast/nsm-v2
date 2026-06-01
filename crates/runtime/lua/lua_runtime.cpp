/// @file lua_runtime.cpp
/// @brief Lua scripting bridge implementation via sol2.

#include "nift/runtime/lua_runtime.hpp"

#include <sol/sol.hpp>
#include <utility>

namespace nift::runtime {

class LuaImpl {
 public:
  sol::state state;

  LuaImpl() {
    // Default sandbox — only safe libraries.
    state.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table,
                         sol::lib::math);
  }

  void enable_unsafe() {
    state.open_libraries(sol::lib::io, sol::lib::os, sol::lib::package, sol::lib::debug,
                         sol::lib::coroutine);
  }
};

LuaRuntime::LuaRuntime() : impl_(std::make_unique<LuaImpl>()) {
}

LuaRuntime::~LuaRuntime() = default;

LuaRuntime::LuaRuntime(LuaRuntime&&) noexcept = default;
LuaRuntime& LuaRuntime::operator=(LuaRuntime&&) noexcept = default;

::nift::Expected<std::string, ::nift::Error> LuaRuntime::exec(std::string_view code) {
  // Capture print output via a string buffer.
  impl_->state["__nift_capture"] = "";
  impl_->state.script(R"(
    __nift_capture = ""
    local _orig_print = print
    print = function(...)
      local args = {...}
      for i = 1, #args do
        if i > 1 then __nift_capture = __nift_capture .. "\t" end
        __nift_capture = __nift_capture .. tostring(args[i])
      end
      __nift_capture = __nift_capture .. "\n"
    end
  )");

  sol::protected_function_result result =
      impl_->state.safe_script(std::string(code), sol::script_pass_on_error);

  if (!result.valid()) {
    return ::nift::unexpected<::nift::Error>(::nift::Error::parse_error);
  }

  std::string captured = impl_->state["__nift_capture"].get_or<std::string>("");

  // If the snippet returned a value, append it.
  if (result.return_count() > 0) {
    sol::object ret = result;
    if (ret.is<std::string>()) {
      captured += ret.as<std::string>();
    } else if (ret.is<double>()) {
      double d = ret.as<double>();
      if (d == static_cast<long long>(d)) {
        captured += std::to_string(static_cast<long long>(d));
      } else {
        captured += std::to_string(d);
      }
    } else if (ret.is<bool>()) {
      captured += ret.as<bool>() ? "true" : "false";
    }
  }

  return captured;
}

void LuaRuntime::set_global(std::string_view name, std::string_view value) {
  impl_->state[std::string(name)] = std::string(value);
}

std::string LuaRuntime::get_global(std::string_view name) const {
  sol::object obj = impl_->state[std::string(name)];
  if (obj.is<std::string>())
    return obj.as<std::string>();
  if (obj.is<double>()) {
    double d = obj.as<double>();
    if (d == static_cast<long long>(d)) {
      return std::to_string(static_cast<long long>(d));
    }
    return std::to_string(d);
  }
  if (obj.is<bool>())
    return obj.as<bool>() ? "true" : "false";
  return "";
}

void LuaRuntime::enable_unsafe() {
  impl_->enable_unsafe();
}

}  // namespace nift::runtime
