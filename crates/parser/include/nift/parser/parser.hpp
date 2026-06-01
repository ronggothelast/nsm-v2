#pragma once
/// @file parser.hpp
/// @brief Parser — converts token stream into AST.

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "nift/parser/ast.hpp"
#include "nift/parser/lexer.hpp"
#include "nift/parser/token.hpp"

namespace nift::parser {

class Parser {
 public:
  Parser(std::vector<Token> tokens, std::string_view filename = "<input>");

  /// Parse all tokens into a Program AST.
  ProgramNode parse();

 private:
  std::vector<Token> tokens_;
  std::string_view filename_;
  size_t pos_ = 0;

  // Token navigation
  const Token& peek() const;
  const Token& advance();
  bool check(TokenType type) const;
  bool is_at_end() const;

  // Argument parsing
  void parse_directive_args(std::string& name, std::vector<std::string>& options,
                            std::vector<std::string>& params);
  bool check_for_brace();

  // Node parsing
  NodePtr parse_node();
  NodePtr parse_text();
  NodePtr parse_directive();
  NodePtr parse_block(const std::string& name, std::vector<std::string> options,
                      std::vector<std::string> params, size_t line);
  std::vector<NodePtr> parse_block_body();
};

/// Parse a template source string into an AST.
ProgramNode parse_template(std::string_view source,
                           std::string_view filename = "<input>");

/// Parse and evaluate a template, returning the output string.
std::string evaluate_template(
    std::string_view source,
    const std::unordered_map<std::string, std::string>& variables = {},
    std::string_view filename = "<input>");

}  // namespace nift::parser
