#pragma once
/// @file quoted.hpp
/// @brief Quoted string parsing — extract strings delimited by quotes,
///        handling escape sequences.

#include <optional>
#include <string>
#include <string_view>

namespace nift::core {

/// Result of parsing a quoted string.
struct QuotedResult {
  std::string content;     ///< The unquoted, unescaped content.
  std::size_t consumed;    ///< Number of characters consumed from input.
};

/// Parse a quoted string starting at position 0 of `sv`.
/// Supports double-quoted ("...") and single-quoted ('...') strings.
/// Recognizes escapes: \\, \", \', \n, \t, \r, \0.
/// Returns nullopt if `sv` doesn't start with a quote character.
[[nodiscard]] std::optional<QuotedResult> parse_quoted(
    std::string_view sv) noexcept;

/// Escape a string for use inside double quotes.
[[nodiscard]] std::string escape_string(std::string_view sv);

/// Remove surrounding quotes if present, no escape processing.
[[nodiscard]] std::string_view unquote(std::string_view sv) noexcept;

}  // namespace nift::core
