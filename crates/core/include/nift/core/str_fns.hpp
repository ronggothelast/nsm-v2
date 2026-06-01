#pragma once
/// @file str_fns.hpp
/// @brief String utility functions — trim, split, replace, case conversion.

#include <string>
#include <string_view>
#include <vector>

namespace nift::core {

/// Trim whitespace from both ends.
[[nodiscard]] std::string_view trim(std::string_view sv) noexcept;

/// Trim whitespace from left.
[[nodiscard]] std::string_view trim_left(std::string_view sv) noexcept;

/// Trim whitespace from right.
[[nodiscard]] std::string_view trim_right(std::string_view sv) noexcept;

/// Split string by delimiter.
[[nodiscard]] std::vector<std::string_view> split(std::string_view sv, char delim);

/// Split string by string delimiter.
[[nodiscard]] std::vector<std::string_view> split(std::string_view sv,
                                                  std::string_view delim);

/// Join strings with separator.
[[nodiscard]] std::string join(const std::vector<std::string_view>& parts,
                               std::string_view sep);

/// Replace all occurrences of `from` with `to` in `str`.
[[nodiscard]] std::string replace_all(std::string_view str, std::string_view from,
                                      std::string_view to);

/// Check if string starts with prefix.
[[nodiscard]] constexpr bool starts_with(std::string_view str,
                                         std::string_view prefix) noexcept {
  if (prefix.size() > str.size())
    return false;
  return str.substr(0, prefix.size()) == prefix;
}

/// Check if string ends with suffix.
[[nodiscard]] constexpr bool ends_with(std::string_view str,
                                       std::string_view suffix) noexcept {
  if (suffix.size() > str.size())
    return false;
  return str.substr(str.size() - suffix.size()) == suffix;
}

/// Check if string contains substring.
[[nodiscard]] constexpr bool contains(std::string_view str,
                                      std::string_view sub) noexcept {
  return str.find(sub) != std::string_view::npos;
}

/// Convert to lowercase (ASCII only).
[[nodiscard]] std::string to_lower(std::string_view sv);

/// Convert to uppercase (ASCII only).
[[nodiscard]] std::string to_upper(std::string_view sv);

}  // namespace nift::core
