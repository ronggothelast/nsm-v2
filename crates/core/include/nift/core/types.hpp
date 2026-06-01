#pragma once
/// @file types.hpp
/// @brief Common type aliases and utilities for nift::core.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// C++23 std::expected shim — use when available, else fall back to a minimal
// implementation sufficient for error propagation.
#if __has_include(<expected>) && __cplusplus > 202002L
#include <expected>
namespace nift {
template <typename T, typename E>
using Expected = std::expected<T, E>;
using std::unexpected;
}  // namespace nift
#else
// Minimal polyfill: enough for nift internal usage.
namespace nift {

template <typename E>
class unexpected {
 public:
  explicit unexpected(E val) : val_(std::move(val)) {}
  [[nodiscard]] const E& value() const& { return val_; }
  [[nodiscard]] E& value() & { return val_; }
  [[nodiscard]] E&& value() && { return std::move(val_); }

 private:
  E val_;
};

template <typename T, typename E>
class Expected {
 public:
  // Success
  Expected(T val) : data_(std::move(val)) {}  // NOLINT(implicit)
  // Error
  Expected(unexpected<E> err) : data_(std::move(err)) {}  // NOLINT(implicit)

  [[nodiscard]] bool has_value() const { return std::holds_alternative<T>(data_); }
  explicit operator bool() const { return has_value(); }

  [[nodiscard]] const T& value() const& { return std::get<T>(data_); }
  [[nodiscard]] T& value() & { return std::get<T>(data_); }
  [[nodiscard]] T&& value() && { return std::get<T>(std::move(data_)); }

  [[nodiscard]] const T& operator*() const& { return value(); }
  [[nodiscard]] T& operator*() & { return value(); }

  [[nodiscard]] const T* operator->() const { return &value(); }
  [[nodiscard]] T* operator->() { return &value(); }

  [[nodiscard]] const E& error() const& {
    return std::get<unexpected<E>>(data_).value();
  }

 private:
  std::variant<T, unexpected<E>> data_;
};

}  // namespace nift
#endif

namespace nift {

/// Standard error type for nift operations.
enum class Error {
  not_found,
  permission_denied,
  already_exists,
  invalid_argument,
  io_error,
  parse_error,
  unknown,
};

/// Convert Error to string_view.
[[nodiscard]] constexpr std::string_view to_string(Error e) noexcept {
  switch (e) {
    case Error::not_found:
      return "not found";
    case Error::permission_denied:
      return "permission denied";
    case Error::already_exists:
      return "already exists";
    case Error::invalid_argument:
      return "invalid argument";
    case Error::io_error:
      return "I/O error";
    case Error::parse_error:
      return "parse error";
    case Error::unknown:
      return "unknown error";
  }
  return "unknown error";
}

/// Validate Error enum is well-formed.
static_assert(static_cast<int>(Error::unknown) == 6,
              "Error enum changed — update to_string() switch");

/// Result type alias — most nift functions return this.
template <typename T>
using Result = Expected<T, Error>;

/// Use for functions that succeed or fail with no return value.
using Status = Result<std::monostate>;

[[nodiscard]] inline Status ok() noexcept {
  return std::monostate{};
}

}  // namespace nift
