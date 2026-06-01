#pragma once
/// @file token.hpp
/// @brief Token types for the Nift template lexer.

#include <string>
#include <string_view>

namespace nift::parser {

/// Token types produced by the lexer.
enum class TokenType {
  Text,          // Literal text content
  Directive,     // @name or @operator (==, !=, <, >, etc.)
  VarSimple,     // $name
  VarBracket,    // $[name]
  VarExpr,       // $`expression`
  LParen,        // ( — inside directive args
  RParen,        // ) — inside directive args
  LBrace,        // { — block open
  RBrace,        // } — block close
  Comma,         // , — argument separator
  LineComment,   // @# ... or @// ...
  BlockComment,  // @#-- ... --# or @/* ... */
  Eof,
};

/// Returns human-readable name for a token type.
const char* token_type_name(TokenType t) noexcept;

/// A single token from the lexer.
struct Token {
  TokenType type;
  std::string value;
  size_t line = 0;
  size_t column = 0;

  /// Check if this token is a structural delimiter.
  [[nodiscard]] constexpr bool is_delimiter() const noexcept {
    switch (type) {
      case TokenType::LParen:
      case TokenType::RParen:
      case TokenType::LBrace:
      case TokenType::RBrace:
      case TokenType::Comma:
        return true;
      default:
        return false;
    }
  }

  /// Check if this token is a variable reference.
  [[nodiscard]] constexpr bool is_variable() const noexcept {
    switch (type) {
      case TokenType::VarSimple:
      case TokenType::VarBracket:
      case TokenType::VarExpr:
        return true;
      default:
        return false;
    }
  }
};

static_assert(sizeof(Token) <= 80, "Token unexpectedly large — check string layout");
static_assert(std::is_default_constructible_v<Token>,
              "Token must be default-constructible");

}  // namespace nift::parser
