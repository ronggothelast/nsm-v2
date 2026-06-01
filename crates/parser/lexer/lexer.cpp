/// @file lexer.cpp
/// @brief Lexer implementation — context-aware tokenization of Nift templates.

#include "nift/parser/lexer.hpp"

#include <algorithm>
#include <cassert>

namespace nift::parser {

// ─── Context enum ─────────────────────────────────────────────────

enum class LexContext {
  Text,       // Default: only @ and $ are special
  Directive,  // After @name: scanning {options} or (params)
  Params,     // Inside (...): commas are separators, everything else text
  Options,    // Inside {...}: commas are separators
};

// ─── Lexer implementation ─────────────────────────────────────────

Lexer::Lexer(std::string_view source, std::string_view filename)
    : source_(source), filename_(filename) {
}

char Lexer::peek() const noexcept {
  return pos_ < source_.size() ? source_[pos_] : '\0';
}

char Lexer::peek_next() const noexcept {
  return pos_ + 1 < source_.size() ? source_[pos_ + 1] : '\0';
}

char Lexer::advance() noexcept {
  char c = source_[pos_++];
  if (c == '\n') {
    ++line_;
    col_ = 1;
  } else {
    ++col_;
  }
  return c;
}

bool Lexer::match(char expected) noexcept {
  if (pos_ < source_.size() && source_[pos_] == expected) {
    advance();
    return true;
  }
  return false;
}

bool Lexer::is_at_end() const noexcept {
  return pos_ >= source_.size();
}

// ─── Text scanning (context-aware) ────────────────────────────────

Token Lexer::scan_text() {
  size_t start = pos_;
  size_t start_line = line_;

  while (!is_at_end()) {
    char c = peek();
    // In text mode, only @ and $ break the text
    // But \@ and \$ are escapes — include both chars as text
    if (c == '\\') {
      char next = peek_next();
      if (next == '@' || next == '$' || next == '\\') {
        advance();
        advance();  // include both \X as text
        continue;
      }
    }
    if (c == '@' || c == '$') {
      break;
    }
    advance();
  }

  if (pos_ == start) {
    return make_token(TokenType::Eof, "", start_line);
  }

  return make_token(TokenType::Text, source_.substr(start, pos_ - start), start_line);
}

// ─── Variable scanning ────────────────────────────────────────────

Token Lexer::scan_variable() {
  size_t start_line = line_;
  advance();  // consume '$'

  if (is_at_end()) {
    return make_token(TokenType::Text, "$", start_line);
  }

  char c = peek();

  // $`expression`
  if (c == '`') {
    advance();  // consume '`'
    size_t expr_start = pos_;
    while (!is_at_end() && peek() != '`') {
      advance();
    }
    auto expr = source_.substr(expr_start, pos_ - expr_start);
    if (!is_at_end())
      advance();  // consume closing '`'
    return make_token(TokenType::VarExpr, expr, start_line);
  }

  // $[name]
  if (c == '[') {
    advance();  // consume '['
    size_t name_start = pos_;
    while (!is_at_end() && peek() != ']') {
      advance();
    }
    auto name = source_.substr(name_start, pos_ - name_start);
    if (!is_at_end())
      advance();  // consume ']'
    return make_token(TokenType::VarBracket, name, start_line);
  }

  // $identifier
  if (is_identifier_start(c)) {
    size_t name_start = pos_;
    while (!is_at_end() && is_identifier_char(peek())) {
      advance();
    }
    return make_token(TokenType::VarSimple,
                      source_.substr(name_start, pos_ - name_start), start_line);
  }

  // Lone $
  return make_token(TokenType::Text, "$", start_line);
}

// ─── Directive scanning ───────────────────────────────────────────

Token Lexer::scan_directive() {
  size_t start_line = line_;
  advance();  // consume '@'

  if (is_at_end()) {
    return make_token(TokenType::Text, "@", start_line);
  }

  char c = peek();

  // @#-- block comment --#
  if (c == '#' && peek_next() == '-') {
    advance();  // consume '#'
    advance();  // consume '-'
    // Check for second -
    if (!is_at_end() && peek() == '-') {
      advance();
    }
    size_t comment_start = pos_;

    // Find --#
    while (!is_at_end()) {
      if (peek() == '-' && pos_ + 2 < source_.size() && source_[pos_ + 1] == '-' &&
          source_[pos_ + 2] == '#') {
        break;
      }
      advance();
    }
    auto comment = source_.substr(comment_start, pos_ - comment_start);
    // Consume --#
    if (!is_at_end()) {
      advance();
      advance();
      advance();
    }
    return make_token(TokenType::BlockComment, comment, start_line);
  }

  // @# line comment
  if (c == '#') {
    advance();  // consume '#'
    size_t comment_start = pos_;
    while (!is_at_end() && peek() != '\n') {
      advance();
    }
    return make_token(TokenType::LineComment,
                      source_.substr(comment_start, pos_ - comment_start), start_line);
  }

  // @// line comment (legacy syntax)
  if (c == '/' && peek_next() == '/') {
    advance();
    advance();  // consume '//'
    size_t comment_start = pos_;
    while (!is_at_end() && peek() != '\n') {
      advance();
    }
    return make_token(TokenType::LineComment,
                      source_.substr(comment_start, pos_ - comment_start), start_line);
  }

  // @/* ... */ C-style comment
  if (c == '/' && peek_next() == '*') {
    advance();
    advance();  // consume '/*'
    size_t comment_start = pos_;
    while (!is_at_end()) {
      if (peek() == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
        break;
      }
      advance();
    }
    auto comment = source_.substr(comment_start, pos_ - comment_start);
    if (!is_at_end()) {
      advance();
      advance();
    }  // consume '*/'
    return make_token(TokenType::BlockComment, comment, start_line);
  }

  // @identifier — directive name
  if (is_identifier_start(c)) {
    size_t name_start = pos_;
    while (!is_at_end() && is_identifier_char(peek())) {
      advance();
    }
    auto name = source_.substr(name_start, pos_ - name_start);

    // Check for multi-char operators: @f++, @n++, @f--, @n--
    if (!is_at_end() && peek() == '+') {
      size_t saved = pos_;
      advance();  // first +
      if (!is_at_end() && peek() == '+') {
        advance();  // second +
        name = "f++";
      } else {
        pos_ = saved;  // backtrack
      }
    } else if (!is_at_end() && peek() == '-') {
      size_t saved = pos_;
      advance();  // first -
      if (!is_at_end() && peek() == '-') {
        advance();  // second -
        name = "f--";
      } else {
        pos_ = saved;  // backtrack
      }
    }

    return make_token(TokenType::Directive, name, start_line);
  }

  // @? — ternary directive
  if (c == '?') {
    advance();
    return make_token(TokenType::Directive, "?", start_line);
  }

  // @... — operator-like directives
  if (c == '=') {
    advance();
    if (!is_at_end() && peek() == '=') {
      advance();
      return make_token(TokenType::Directive, "==", start_line);
    }
    return make_token(TokenType::Directive, "=", start_line);
  }
  if (c == '!') {
    advance();
    if (!is_at_end() && peek() == '=') {
      advance();
      return make_token(TokenType::Directive, "!=", start_line);
    }
    return make_token(TokenType::Directive, "!", start_line);
  }
  if (c == '<') {
    advance();
    if (!is_at_end() && peek() == '=') {
      advance();
      return make_token(TokenType::Directive, "<=", start_line);
    }
    return make_token(TokenType::Directive, "<", start_line);
  }
  if (c == '>') {
    advance();
    if (!is_at_end() && peek() == '=') {
      advance();
      return make_token(TokenType::Directive, ">=", start_line);
    }
    return make_token(TokenType::Directive, ">", start_line);
  }

  // Assignment operators
  if (c == ':') {
    advance();
    if (!is_at_end() && peek() == '=') {
      advance();
      return make_token(TokenType::Directive, ":=", start_line);
    }
    return make_token(TokenType::Text, "@:", start_line);
  }

  // Math operators
  if (c == '+') {
    advance();
    return make_token(TokenType::Directive, "+", start_line);
  }
  if (c == '-') {
    advance();
    return make_token(TokenType::Directive, "-", start_line);
  }
  if (c == '*') {
    advance();
    return make_token(TokenType::Directive, "*", start_line);
  }
  if (c == '/') {
    advance();
    return make_token(TokenType::Directive, "/", start_line);
  }
  if (c == '%') {
    advance();
    return make_token(TokenType::Directive, "%", start_line);
  }
  if (c == '|') {
    advance();
    return make_token(TokenType::Directive, "|", start_line);
  }

  // Literal @
  return make_token(TokenType::Text, "@", start_line);
}

// ─── Parameter scanning ───────────────────────────────────────────

Token Lexer::scan_param_text() {
  size_t start = pos_;
  size_t start_line = line_;
  int paren_depth = 0;

  while (!is_at_end()) {
    char c = peek();

    // Handle paren nesting
    if (c == '(') {
      ++paren_depth;
      advance();
      continue;
    }
    if (c == ')') {
      if (paren_depth == 0)
        break;  // closing paren of our arg list
      --paren_depth;
      advance();
      continue;
    }
    if (c == ',')
      break;  // argument separator

    advance();
  }

  if (pos_ == start) {
    return make_token(TokenType::Eof, "", start_line);
  }

  return make_token(TokenType::Text, source_.substr(start, pos_ - start), start_line);
}

// ─── Helpers ──────────────────────────────────────────────────────

Token Lexer::make_token(TokenType type, std::string_view value, size_t line) const {
  return Token{type, std::string(value), line, 0};
}

bool Lexer::is_identifier_start(char c) noexcept {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::is_identifier_char(char c) noexcept {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// ─── Main tokenize ────────────────────────────────────────────────

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;

  while (!is_at_end()) {
    char c = peek();

    // Variable interpolation
    if (c == '$') {
      tokens.push_back(scan_variable());
      continue;
    }

    // Directive
    if (c == '@') {
      tokens.push_back(scan_directive());
      continue;
    }

    // Text (everything else in text mode)
    tokens.push_back(scan_text());
  }

  tokens.push_back(make_token(TokenType::Eof, "", line_));
  return tokens;
}

// ─── Utility ──────────────────────────────────────────────────────

bool Lexer::is_whitespace_only(std::string_view sv) noexcept {
  for (char c : sv) {
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      return false;
    }
  }
  return true;
}

}  // namespace nift::parser
