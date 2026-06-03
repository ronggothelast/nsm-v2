#pragma once
/// @file lexer.hpp
/// @brief Context-aware lexer for Nift templates.
///
/// In Text mode, only @ and $ are special. Everything else (commas, braces,
/// operators, newlines) is literal text.
/// When a directive is found, the parser handles {options} and (params) parsing.

#include <string>
#include <string_view>
#include <vector>

#include "nift/parser/token.hpp"

namespace nift::parser {

class Lexer {
 public:
  explicit Lexer(std::string_view source, std::string_view filename = "<input>");

  /// Tokenize the entire input.
  std::vector<Token> tokenize();

  /// Check if a string is whitespace-only.
  static bool is_whitespace_only(std::string_view sv) noexcept;

  /// Character classification helpers (public for parser use).
  static bool is_identifier_start(char c) noexcept;
  static bool is_identifier_char(char c) noexcept;

 private:
  std::string_view source_;
  std::string_view filename_;
  size_t pos_ = 0;
  size_t line_ = 1;
  size_t col_ = 1;

  // Character access
  [[nodiscard]] char peek() const noexcept;
  [[nodiscard]] char peek_next() const noexcept;
  char advance() noexcept;
  bool match(char expected) noexcept;
  [[nodiscard]] bool is_at_end() const noexcept;

  // Scanning modes
  Token scan_text();
  Token scan_variable();
  Token scan_directive();
  Token scan_param_text();

  // Directive sub-scanners (called by scan_directive after '@' is consumed)
  Token scan_comment(size_t start_line);
  Token scan_operator(size_t start_line);

  // Helpers
  Token make_token(TokenType type, std::string_view value, size_t line) const;
};

}  // namespace nift::parser
